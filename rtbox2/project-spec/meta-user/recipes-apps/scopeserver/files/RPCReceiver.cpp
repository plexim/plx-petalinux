/*
 * Copyright (c) 2020 Plexim GmbH.
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
#include <QtCore/QDir>
#include "spinlock.h"

#define member_size(type, member) sizeof(((type *)0)->member)

struct MsgQueueInfo
{
   uint8_t mHead;
   uint8_t mTail;
   uint16_t mPad1;
   uint32_t mPad2;
};

struct Msg
{
   uint32_t mLength;
   uint32_t mPadding;
   uint8_t mMsg[504];
};

struct MsgQueue
{
   struct MsgQueueInfo mInfo;
   struct Msg mMsgs[256];
};

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

QString RPCReceiver::findUIODevice()
{
   QDir uioDir("/sys/devices/platform/fc000000.pci/pci0000:00/0000:00:00.0/uio");
   QStringList uioName = uioDir.entryList(QStringList() << "uio*");
   return uioName.empty() ? QString() : uioName[0]; 
}

RPCReceiver::RPCReceiver()
 : QObject(),
   mSimulationConnection(),
   mNotifier(nullptr),
   mSendQueue(nullptr),
   mReceiveQueue(nullptr),
   mScopeBuffer(nullptr),
   mCurrentMessageFilter(0),
   mCpuPerformanceCounters(nullptr)
{
   QString uioDev = RPCReceiver::findUIODevice();
   if (uioDev.isEmpty())
	qCritical("UIO device not found.");
   mSimulationConnection.setFileName(QString("/dev/") + uioDev);
   qDebug() << "Connecting to" << mSimulationConnection.fileName();
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
   if (!reconnect())
      qFatal("Connection failed, exiting.");

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

#define dataAvailable() (mReceiveQueue->mInfo.mHead != mReceiveQueue->mInfo.mTail)

void RPCReceiver::receiveData()
{
   // qWarning("receiveData");
   SocketNotifierHelper helper(mNotifier);
   int buf;
   mSimulationConnection.read((char*)&buf, sizeof(buf));
   do {
      if (!dataAvailable())
         return;
      int msg = *(int*)mReceiveQueue->mMsgs[mReceiveQueue->mInfo.mTail].mMsg;
      // qDebug() << "Received" << msg;
      QMutexLocker locker(&mMutex);
      if (mCurrentMessageFilter && mCurrentMessageFilter == msg)
      {
         if (readMsgData(mCurrentMessage))
         {
            mCurrentMessageFilter = 0;
            locker.unlock();
            mWaitCondition.wakeAll();
         }
      }
      else
      {
         // qDebug() << "msg" << msg << head << tail;
         processMessage(msg);
      }
      mReceiveQueue->mInfo.mTail++;
   } while (dataAvailable());
}

bool RPCReceiver::readMsgData(QByteArray& aData)
{
   const int msgLength = *(((int*)mReceiveQueue->mMsgs[mReceiveQueue->mInfo.mTail].mMsg) + 1);
   aData.clear();
   aData.reserve(msgLength+1);
   QByteArray chunk;
   int readBytes = readTail(chunk) - 2*sizeof(int);
   aData.append(chunk.mid(2*sizeof(int)));
   while (readBytes < msgLength)
   {
      if (!dataAvailable())
      {
         // wait for data
         int buf;
         mSimulationConnection.read((char*)&buf, sizeof(buf));
      }
      mReceiveQueue->mInfo.mTail++;
      int size = readTail(chunk);
      if (size <= 0)
         return false;
      readBytes += size;
      aData.append(chunk);
   }
   return true;
}

int RPCReceiver::readTail(QByteArray& aData)
{
   if (!mReceiveQueue)
      return -1;
   uint8_t tail = mReceiveQueue->mInfo.mTail;
   int size = mReceiveQueue->mMsgs[tail].mLength;
   if (size > 0)
   {
      aData.resize(size);
      //qDebug() << tail << size << (void*)aData.data() << mReceiveQueue->mMsgs[tail].mMsg+2*sizeof(int);
      memcpy(aData.data(), mReceiveQueue->mMsgs[tail].mMsg, size);
   }
   return size;
}

bool RPCReceiver::processMessage(int aMessage)
{
    switch (aMessage)
    {
       case SimulationRPC::RSP_SCOPE_DATA_READY:
       {
          QByteArray buffer;
          readMsgData(buffer);
          emit scopeArmResponse(buffer);
          break;
       }
       case SimulationRPC::RSP_TUNE_PARAMETERS:
       {
          const int errorCode = *(((int*)mReceiveQueue->mMsgs[mReceiveQueue->mInfo.mTail].mMsg) + 1);
          emit tuneParameterResponse(errorCode);
          break;
       }
       case SimulationRPC::NOTIFICATION_INIT_COMPLETE:
          log(QString("Model initialization complete."));
          emit initComplete();
          break;
       case SimulationRPC::CAN_INIT:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             const struct SimulationRPC::CanSetupMsg* canMsg = (const struct SimulationRPC::CanSetupMsg*)buffer.data();
             if (canMsg->mModuleId < sizeof(mCanHandlers)/sizeof(CanHandler*))
                mCanHandlers[canMsg->mModuleId]->canInit(*canMsg);
          }
          break;
       }
       case SimulationRPC::CAN_TRANSMIT:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             const struct SimulationRPC::CanTransmitMsg* canMsg = (const struct SimulationRPC::CanTransmitMsg*)buffer.data();
             if (canMsg->mModuleId < sizeof(mCanHandlers)/sizeof(CanHandler*))
                mCanHandlers[canMsg->mModuleId]->canTransmit(*canMsg);
          }
          break;
       }
       case SimulationRPC::CAN_REQUEST_ID:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             const struct SimulationRPC::CanRequestIdMsg* canMsg = (const struct SimulationRPC::CanRequestIdMsg*)buffer.data();
             if (canMsg->mModuleId < sizeof(mCanHandlers)/sizeof(CanHandler*))
                mCanHandlers[canMsg->mModuleId]->canRequestId(*canMsg);
          }
          break;
       }
       case SimulationRPC::UDP_SEND:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             const struct SimulationRPC::UdpSendMsg* sendMsg = (const struct SimulationRPC::UdpSendMsg*)buffer.data();
             mUdpTxHandler->sendData(sendMsg->mData, sendMsg->mSize, sendMsg->mAddress, sendMsg->mPort);
          }
          break;
       }
       case SimulationRPC::UDP_REQUEST_PORTS:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             const struct SimulationRPC::UdpRequestPortsMsg* udpMsg = (const struct SimulationRPC::UdpRequestPortsMsg*)buffer.data();
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
       case SimulationRPC::MSG_LOG:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             int flags = *(int*)buffer.data();
             emit logMessage(flags, buffer.mid(sizeof(int)));
          }
          break;
       }
       default:
          log(QString("Unexpected message from simulation core %1").arg(aMessage));
          return false;
    }
    return true;
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
   int tryResend = 0;
   int sendError = 0;
   do
   {
      {
         SocketNotifierHelper helper(mNotifier);
         int totalMsgLength = aData.size();
         int bytesSent = 0;
         while (totalMsgLength > 0)
         {
            int chunkSize = qMin(totalMsgLength, 496);
            if (!sendMsg(aData.data() + bytesSent, chunkSize))
            {
               qWarning("Can't send message to simulation core.");
               if (mMutex.tryLock(100))
                  mWaitCondition.wakeAll();
               sendError = 1;
               break;
            }
            bytesSent += chunkSize;
            totalMsgLength -= chunkSize;
         }
      }
      if (sendError)
      {
         tryResend = !tryResend;
         if (tryResend)
            reconnect();
         else
         {
            qWarning("Sending after reconnecting failed, discarding message.");
         }
      }
   } while (tryResend && mNotifier);
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
   const int shutdownMsg = 0xef56a55a;
   sendMsg((const char*)&shutdownMsg, sizeof(int));
   emit finished();
}

bool RPCReceiver::reconnect()
{
   close();
   if (!mSimulationConnection.open(QIODevice::ReadWrite | QIODevice::Unbuffered))
   {
      emit error(tr("Can't open connection to simulation core: %1").arg(mSimulationConnection.errorString()));
      return false;
   }
   mSendQueue = (struct MsgQueue*)mSimulationConnection.map(
      2*4096,
      ((2*sizeof(struct MsgQueue)-1)/4096+1)*4096 + 0x2080000,
      QFileDevice::NoOptions
   );

   if (!mSendQueue)
   {
      mSimulationConnection.close();
      emit error(tr("Cannot map memory."));
      return false;
   }
   mReceiveQueue = mSendQueue + 1;
   memset(&mReceiveQueue->mInfo, 0, sizeof(mSendQueue->mInfo));
   mCpuPerformanceCounters = (uint32_t*)(mReceiveQueue + 1);
   mNotifier = new QSocketNotifier(mSimulationConnection.handle(), QSocketNotifier::Read, this);
   connect(mNotifier, SIGNAL(activated(int)), this, SLOT(receiveData()));
   return true;
}


bool RPCReceiver::mapBuffers(int aScopeBufferSize, int aRxTxBufferSize)
{
   if (!mReceiveQueue)
   {
      return false;
   }
   mScopeBuffer = ((char*)mSendQueue) + ((2*sizeof(struct MsgQueue)+3*sizeof(uint32_t)-1)/4096+1)*4096;
   mRxBuffer = (volatile char*)mScopeBuffer + aScopeBufferSize;
   mTxBuffer = (char*)mRxBuffer + aRxTxBufferSize/2;
   return true;
}


void RPCReceiver::close()
{
   if (mNotifier)
   {
      delete mNotifier;
      mNotifier = nullptr;
   }
   if (mReceiveQueue)
   {
      mSimulationConnection.unmap((uchar*)mReceiveQueue);
      mReceiveQueue = nullptr;
   }
   mSendQueue = nullptr;
   mScopeBuffer = nullptr;
   mRxBuffer = nullptr;
   mTxBuffer = nullptr;
   mCpuPerformanceCounters = nullptr;
   mSimulationConnection.close();
}

bool RPCReceiver::sendMsg(const void* aData, int aLength)
{
   //qDebug() << "sending" << *(int*)aData << aLength;
   if ((size_t)aLength > member_size(struct Msg, mMsg))
      aLength = member_size(struct Msg, mMsg);
   uint8_t head = mSendQueue->mInfo.mHead;
   if (head+1 == mSendQueue->mInfo.mTail)
      return false; // no more free buffer
   mSendQueue->mMsgs[head].mLength = aLength;
   memcpy(mSendQueue->mMsgs[head].mMsg, aData, aLength);
   // memory barrier
   asm volatile("dmb ish" ::: "memory");
   mSendQueue->mInfo.mHead++;
   return true;
}


