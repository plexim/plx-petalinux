/*
 * Copyright (c) 2018-2020 Plexim GmbH.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 3, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "RPCReceiver.h"
#include "SimulationRPC.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include "CanHandler.h"
#include "UdpTxHandler.h"
#include "UdpRxHandler.h"
#include <QtCore/QSocketNotifier>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>

#define IOC_GET_QUEUE_LENGTH	        _IOR('z', 1, unsigned int)
#define IOC_GET_CURRENT_BUFFER_LENGTH   _IOR('z', 2, unsigned int)

class SocketNotifierHelper
{
public:
   SocketNotifierHelper(QSocketNotifier* aNotifier) :
      mNotifier(aNotifier),
      mOldState(aNotifier->isEnabled())
   {
      mNotifier->setEnabled(false);
   }
   ~SocketNotifierHelper() { mNotifier->setEnabled(mOldState); }

private:
   QSocketNotifier* mNotifier;
   bool mOldState;
};


RPCReceiver::RPCReceiver()
 : QObject(),
   mSimulationConnection("/dev/rpmsg0"),
   mNotifier(nullptr),
   mCurrentMessageFilter(0)
{
   for (int i=0; i<2; i++)
      mCanHandlers[i] = nullptr;
   connect(this, &RPCReceiver::sendRequest, this, &RPCReceiver::send, Qt::QueuedConnection);
}

RPCReceiver::~RPCReceiver()
{
   for (int i=0; i<2; i++)
   {
      if (mCanHandlers[i])
         mCanHandlers[i]->canStop();
   }
   for (auto it = mUdpRxHandlers.begin(); it != mUdpRxHandlers.end(); ++it)
   {
      (*it)->stop();
   }
   if (mNotifier)
   {
      delete mNotifier;
      mNotifier = nullptr;
   }
   if (mSimulationConnection.exists() && mSimulationConnection.isOpen())
   {
      mSimulationConnection.close();
   }
}


void RPCReceiver::process()
{
   openConnection();

   for (int i=0; i<2; i++)
      mCanHandlers[i] = new CanHandler(i, this);
   mUdpTxHandler = new UdpTxHandler(this);
}


bool RPCReceiver::waitForMessage(char* aSendMsg, int aMsgSize, int aMessageId, QByteArray& aData, unsigned long aTimeout)
{
   if (!mSimulationConnection.isOpen())
      return false;
   mMutex.lock();
   mCurrentMessageFilter = aMessageId;
   emit sendRequest(QByteArray(aSendMsg, aMsgSize));
   // qWarning("wait start");
   bool ret = mWaitCondition.wait(&mMutex, aTimeout);
   // qWarning("wait end");
   if (ret)
      aData = mCurrentMessage;
   mCurrentMessageFilter = 0;
   mMutex.unlock();
   return ret;
}


void RPCReceiver::receiveData()
{
   unsigned int bufferSize = 0;
   SocketNotifierHelper helper(mNotifier);
   while (ioctl(mSimulationConnection.handle(), IOC_GET_CURRENT_BUFFER_LENGTH, &bufferSize) == 0 && bufferSize)
   {
      // qWarning() << "Buffer size:" << bufferSize;
      QByteArray msgBuf;
      msgBuf.resize(bufferSize);
      int size = mSimulationConnection.read(msgBuf.data(), bufferSize);
      int msg = *((int*)msgBuf.data());
      // qWarning() << "Received" << msg << "initial buf size" << size;
      QMutexLocker locker(&mMutex);
      if (mCurrentMessageFilter && mCurrentMessageFilter == msg)
      {
         mCurrentMessage = msgBuf;
         if (readData(mCurrentMessage))
         {
            mCurrentMessageFilter = 0;
            locker.unlock();
            mWaitCondition.wakeAll();
         }
      }
      else
      {
         processMessage(msg, msgBuf);
      }
   }
}

bool RPCReceiver::processMessage(int aMessage, QByteArray& aMsgBuf)
{
    switch (aMessage)
    {
       case SimulationRPC::RSP_SCOPE_DATA_READY:
       {
          readData(aMsgBuf);
          emit scopeArmResponse(aMsgBuf);
          break;
       }
       case SimulationRPC::RSP_TUNE_PARAMETERS:
          qint32 errorCode;
          if (aMsgBuf.size() < 2*sizeof(qint32))
          {
             log(QString("Cannot read tune parameters response."));
             return false;
          }
          errorCode = *((qint32*)aMsgBuf.data()+1);
          emit tuneParameterResponse(errorCode);
          break;
       case SimulationRPC::NOTIFICATION_INIT_COMPLETE:
          log(QString("Model initialization complete."));
          emit initComplete();
          break;
       case SimulationRPC::CAN_INIT:
       {
          if (readData(aMsgBuf))
          {
             const struct SimulationRPC::CanSetupMsg* canMsg = (const struct SimulationRPC::CanSetupMsg*)aMsgBuf.data();
             if (canMsg->mModuleId < sizeof(mCanHandlers)/sizeof(CanHandler*))
                mCanHandlers[canMsg->mModuleId]->canInit(*canMsg);
          }
          break;
       }
       case SimulationRPC::CAN_TRANSMIT:
       {
          if (readData(aMsgBuf))
          {
             const struct SimulationRPC::CanTransmitMsg* canMsg = (const struct SimulationRPC::CanTransmitMsg*)aMsgBuf.data();
             if (canMsg->mModuleId < sizeof(mCanHandlers)/sizeof(CanHandler*))
                mCanHandlers[canMsg->mModuleId]->canTransmit(*canMsg);
          }
          break;
       }
       case SimulationRPC::CAN_REQUEST_ID:
       {
          if (readData(aMsgBuf))
          {
             const struct SimulationRPC::CanRequestIdMsg* canMsg = (const struct SimulationRPC::CanRequestIdMsg*)aMsgBuf.data();
             if (canMsg->mModuleId < sizeof(mCanHandlers)/sizeof(CanHandler*))
                mCanHandlers[canMsg->mModuleId]->canRequestId(*canMsg);
          }
          break;
       }
       case SimulationRPC::UDP_SEND:
       {
          if (readData(aMsgBuf))
          {
             const struct SimulationRPC::UdpSendMsg* sendMsg = (const struct SimulationRPC::UdpSendMsg*)aMsgBuf.data();
             mUdpTxHandler->sendData(sendMsg->mData, sendMsg->mSize, sendMsg->mAddress, sendMsg->mPort);
          }
          break;
       }
       case SimulationRPC::UDP_REQUEST_PORTS:
       {
          if (readData(aMsgBuf))
          {
             const struct SimulationRPC::UdpRequestPortsMsg* udpMsg = (const struct SimulationRPC::UdpRequestPortsMsg*)aMsgBuf.data();
             for (uint32_t i=0; i<udpMsg->mNumPorts; i++)
             {
                bool found = false;
                for (auto it = mUdpRxHandlers.begin(); it != mUdpRxHandlers.end(); ++it)
                {
                   if ((*it)->getPort() == udpMsg->mPorts[i])
                   {
                      found = true;
                      break;
                   }
                }
                if (!found)
                {
                   UdpRxHandler* newHandler = new UdpRxHandler(udpMsg->mPorts[i], this);
                   mUdpRxHandlers.push_back(newHandler);
                }
             }
          }
          break;
       }
       case SimulationRPC::MSG_ERROR:
          emit simulationError();
          break;
       default:
          log(QString("Unexpected message from simulation core %1").arg(aMessage));
          readAll();
          return false;
    }
    return true;
}

bool RPCReceiver::readData(QByteArray& aData)
{
   if (aData.size() < 2*sizeof(int))
      return false;
   int totalSize = *((int*)aData.data() + 1);
   aData = aData.mid(2*sizeof(int));
   qint64 readBytes = aData.size();
   aData.resize(totalSize);
   totalSize -= readBytes;
   while (totalSize > 0)
   {
      readBytes = mSimulationConnection.read(aData.data() + readBytes, totalSize);
      if (readBytes < 0)
      {
         qWarning("Error reading from simulation core.");
         return false;
      }
      totalSize -= readBytes;
   }
   return true;
}

void RPCReceiver::readAll()
{
   unsigned int bufferSize = 0;
   while (ioctl(mSimulationConnection.handle(), IOC_GET_CURRENT_BUFFER_LENGTH, &bufferSize) == 0 &&
       bufferSize)
   {
      mSimulationConnection.read(bufferSize);
   }
}

void RPCReceiver::log(const QString &aMsg)
{
   emit sigLog(aMsg);
}

void RPCReceiver::reportError(const QString &aMsg)
{
   emit sigError(aMsg);
}

void RPCReceiver::send(QByteArray aData)
{
   if (!mSimulationConnection.isOpen())
      return;
   int sendError = 0;
   {
      SocketNotifierHelper helper(mNotifier);
      int totalMsgLength = aData.size();
      int bytesSent = 0;
      while (totalMsgLength > 0 && !sendError)
      {
         int chunkSize = qMin(totalMsgLength, 496);
         if (mSimulationConnection.write(aData.data() + bytesSent, chunkSize) != chunkSize)
         {
            qWarning("Can't send message to simulation core.");
            if (mMutex.tryLock(100))
            mWaitCondition.wakeAll();
            sendError = 1;
         }
         bytesSent += chunkSize;
         totalMsgLength -= chunkSize;
      }
      if (sendError)
      {
            qWarning("Sending after reconnecting failed, discarding message.");
      }
   }
   if (mNotifier)
      receiveData();
}

void RPCReceiver::shutdown()
{
   for (int i=0; i<2; i++)
   {
      if (mCanHandlers[i])
      {
         mCanHandlers[i]->canStop();
         mCanHandlers[i]->deleteLater();
         mCanHandlers[i] = nullptr;
      }
   }
   for (auto it = mUdpRxHandlers.begin(); it != mUdpRxHandlers.end(); ++it)
   {
      (*it)->stop();
      (*it)->deleteLater();
   }
   mUdpRxHandlers.clear();
   if (!mSimulationConnection.isOpen())
      return;
   if (mNotifier)
   {
      delete mNotifier;
      mNotifier = nullptr;
   }
   const int shutdownMsg = 0xef56a55a;
   mSimulationConnection.write((const char*)&shutdownMsg, sizeof(int));
   emit finished();
}

void RPCReceiver::openConnection()
{
   if (!mSimulationConnection.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
   {
      emit error(tr("Can't open connection to simulation core: %1").arg(mSimulationConnection.errorString()));
      return;
   }
   mNotifier = new QSocketNotifier(mSimulationConnection.handle(), QSocketNotifier::Read, this);
   connect(mNotifier, SIGNAL(activated(int)), this, SLOT(receiveData()));
}

