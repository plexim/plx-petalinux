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
#include "XcpHandler.h"
#include "xcpTl.h"
#include <QtCore/QSocketNotifier>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtCore/QDir>
#include "spinlock.h"
#include "ToFileHandler.h"
#include <QElapsedTimer>
#include <QDebug>
#include <QtNetwork/QHostInfo>

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
   QDir pciDir("/sys/devices/platform/fc000000.pci");
   QStringList pciName = pciDir.entryList(QStringList() << "pci*");
   if (pciName.empty())
      return QString();
   QDir pciDevDir(pciDir.filePath(pciName[0]));
   QStringList pciDevName = pciDevDir.entryList(QStringList() << "00*");
   if (pciDevName.empty())
      return QString();
   QDir uioDir(pciDevDir.filePath(pciDevName[0])+"/uio");
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
   mCpuPerformanceCounters(nullptr),
   mXcpHandler(nullptr)
{
   QString uioDev = RPCReceiver::findUIODevice();
   if (uioDev.isEmpty())
	qCritical("UIO device not found.");
   mSimulationConnection.setFileName(QString("/dev/") + uioDev);
   qDebug() << "Connecting to" << mSimulationConnection.fileName();
   for (int i=0; i<2; i++)
      mCanHandlers[i] = nullptr;
   connect(this, &RPCReceiver::sendRequest, this, &RPCReceiver::send, Qt::QueuedConnection);
   ToFileHandler::staticInit();
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
   if (mXcpHandler)
      mXcpHandler->stop();
   for (auto it = mToFileHandlers.begin(); it != mToFileHandlers.end(); ++it)
   {
      (*it)->stop(false);
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
   mXcpHandler = new XcpHandler(5555, this);
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
       case SimulationRPC::NOTIFICATION_INIT_STARTED:
          log(QString("Model initialization started."));
          emit initStarted();
          break;
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
                mCanHandlers[canMsg->mModuleId]->canInit(*canMsg, buffer.size());
          }
          break;
       }
       case SimulationRPC::CAN_TRANSMIT:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             const struct SimulationRPC::CanTransmitMsg* canMsg = (const struct SimulationRPC::CanTransmitMsg*)buffer.data();
             if (canMsg->mModuleId < sizeof(mCanHandlers)/sizeof(CanHandler*) &&
                 mCanHandlers[canMsg->mModuleId])
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
       case SimulationRPC::TO_FILE_BUFFER_FULL:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             const struct SimulationRPC::ToFileBufferFullMsg* toFileMsg = (const struct SimulationRPC::ToFileBufferFullMsg*)buffer.data();
             if(toFileMsg->mInstance < mToFileHandlers.size())
             {
                mToFileHandlers[toFileMsg->mInstance]->writeToFileBuffer(toFileMsg->mCurrentReadBuffer,
                                                                         toFileMsg->mBufferLength);
             }
          }
          break;
       }
       case SimulationRPC::MSG_TO_FILE_ROTATE:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             const struct SimulationRPC::ToFileRotateMsg* rotateMsg = (const struct SimulationRPC::ToFileRotateMsg*)buffer.data();
             if(rotateMsg->mInstance < mToFileHandlers.size())
             {
                mToFileHandlers[rotateMsg->mInstance]->rotateFile();
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
       case SimulationRPC::MSG_ERROR:
          for (auto it = mToFileHandlers.begin(); it != mToFileHandlers.end(); ++it)
          {
             (*it)->stop(false);
          }
          emit simulationError();
          break;
       case SimulationRPC::RSP_XCP_SEND_DTO:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             unsigned int bufferIndex = *(unsigned int*)buffer.data();
             if (bufferIndex < XCPTL_DTO_QUEUE_SIZE)
             	udpTlSendDtoPacket(mXcpDtoBuffer[bufferIndex]);
          }
          break;
       }
       case SimulationRPC::RSP_XCP_SEND_CTO:
          udpTlSendCrmPacket(mXcpCtoBuffer);
          break;
       case SimulationRPC::NOTIFICATION_INIT_ETHERCAT:
       {
          QByteArray buffer;
          log(QString("Model initialization complete."));
          readMsgData(buffer);
          emit initEthercat(*((const int*)buffer.constData()));
          break;
       }
       case SimulationRPC::MSG_RESOLVE_HOSTNAME:
       {
          QByteArray buffer;
          if (readMsgData(buffer))
          {
             log(QString("Resolving hostname '%1'").arg(buffer.data()));
             QHostInfo::lookupHost(buffer.data(), this, &RPCReceiver::hostnameResolved);
          }
          break;
       }
       default:
          log(QString("Unexpected message from simulation core %1").arg(aMessage));
          return false;
    }
    return true;
}

void RPCReceiver::hostnameResolved(QHostInfo aInfo)
{
   QByteArray buf;
   buf.resize(sizeof(struct HostnameResponse));
   struct HostnameResponse* resp = (struct HostnameResponse*)buf.data();
   resp->mMsg = SimulationRPC::RSP_RESOLVE_HOSTNAME;
   resp->mMsgLength = buf.size();
   resp->mErrorCode = 0;
   resp->mIp = 0;
   log(QString("Hostname resolved"));
   if (aInfo.error() == QHostInfo::NoError)
   {
      for (const auto& addr : aInfo.addresses())
      {
         if (addr.protocol() == QAbstractSocket::IPv4Protocol)
         {
            resp->mIp = addr.toIPv4Address();
            log(QString("IP = %1").arg(addr.toString()));
            break;                   
         }
      }
      if (!resp->mIp)
         resp->mErrorCode = 3; // no IPV4 address found;                
   }
   else
      resp->mErrorCode = aInfo.error();
   send(buf);
}

void RPCReceiver::log(const QString& aMsg)
{
   emit sigLog(aMsg);
}

void RPCReceiver::reportError(const QString& aMsg)
{
   emit sigError(aMsg);
}

void RPCReceiver::reportErrorMessage(const QString& aMsg)
{
   emit logMessage(1, aMsg.toLocal8Bit());
}

void RPCReceiver::send(const QByteArray& aData)
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
            if (!sendMsg(aData.constData() + bytesSent, chunkSize))
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

   for (auto it = mToFileHandlers.begin(); it != mToFileHandlers.end(); ++it)
   {
      (*it)->stop(false);
      (*it)->deleteLater();
   }
   mToFileHandlers.clear();
   mXcpHandler->stop();
   mXcpHandler->deleteLater();
   mXcpHandler = nullptr;
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
      ((2*sizeof(struct MsgQueue)-1)/4096+1)*4096,// + 0x2880000,
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
   mNotifier = new QSocketNotifier(mSimulationConnection.handle(), QSocketNotifier::Read, this);
   connect(mNotifier, SIGNAL(activated(int)), this, SLOT(receiveData()));
   return true;
}


bool RPCReceiver::mapBuffers(int aScopeBufferSize, int aToFileBufferSize, int aRxTxBufferSize)
{
   if (!mSimulationConnection.isOpen())
   {
      return false;
   }
   if (mSendQueue)
   {
      mSimulationConnection.unmap((uchar*)mSendQueue);
   }
   mSendQueue = (struct MsgQueue*)mSimulationConnection.map(
      2*4096,
      ((2*sizeof(struct MsgQueue)-1)/4096+1)*4096 + aScopeBufferSize + aToFileBufferSize + aRxTxBufferSize +
      XCPTL_DTO_QUEUE_SIZE * 8192 + ((sizeof(tXcpCtoMessage)-1)/4096+1)*4096,
      QFileDevice::NoOptions
   );

   if (!mSendQueue)
   {
      mSimulationConnection.close();
      emit error(tr("Cannot map memory."));
      return false;
   }
   mReceiveQueue = mSendQueue + 1;
   mCpuPerformanceCounters = (uint32_t*)(mReceiveQueue + 1);
   mScopeBuffer = ((char*)mSendQueue) + ((2*sizeof(struct MsgQueue)+3*sizeof(uint32_t)-1)/4096+1)*4096;
   mToFileBuffer = mScopeBuffer + aScopeBufferSize;
   mRxBuffer = (volatile char*)mToFileBuffer + aToFileBufferSize;
   mTxBuffer = (char*)mRxBuffer + aRxTxBufferSize/2;
   for (int i=0; i<XCPTL_DTO_QUEUE_SIZE; i++)
      mXcpDtoBuffer[i] = (void*)mRxBuffer + aRxTxBufferSize + 8192*i;
   mXcpCtoBuffer = (void*)mRxBuffer + aRxTxBufferSize + 8192*XCPTL_DTO_QUEUE_SIZE;
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
   mToFileBuffer = nullptr;
   mCpuPerformanceCounters = nullptr;
   mSimulationConnection.close();
}

bool RPCReceiver::sendMsg(const char* aData, int aLength)
{
   //qDebug() << "sending" << *(int*)aData << aLength;
   if ((size_t)aLength > member_size(struct Msg, mMsg))
      aLength = member_size(struct Msg, mMsg);
   uint8_t head = mSendQueue->mInfo.mHead;
   if (head+1 == mSendQueue->mInfo.mTail)
      return false; // no more free buffer
   mSendQueue->mMsgs[head].mLength = aLength;
   memcpy(mSendQueue->mMsgs[head].mMsg, aData, aLength);
   mSendQueue->mInfo.mHead++;
   return true;
}

void RPCReceiver::initializeToFileHandler(QString aFileName, QByteArray aModelName, int aWidth, int aNumSamples, 
                                          int aBufferOffset, int aFileType, int aWriteDevice, bool aUseDouble)
{
   QString modelName = QString(aModelName);
   ToFileHandler* newToFileHandler = new ToFileHandler(modelName, aFileName, aWidth, aNumSamples, aBufferOffset, aFileType, aWriteDevice, aUseDouble, this);
   mToFileHandlers.push_back(newToFileHandler);
}

