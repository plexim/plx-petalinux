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

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QTime>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QtCore/QVariant>
#include <QtCore/QThread>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtCore/QCoreApplication>
#include "Server.h"
#include "ReleaseInfo.h"
#include "xml-rpc/RTBoxError.h"
#include "RPCReceiver.h"
#include "FanControl.h"
#include <unistd.h>
#include <cmath>
#include <sys/mman.h>
#include "PerformanceCounter.h"

#include <QtCore/QDebug>

#define MY_API_VERSION 4

struct QueryAPIMsg {
   int msg;
   struct SimulationRPC::VersionType mFirmwareVersion;
};

struct MsgHdr
{
   int mMsg;
   int mMsgLength;
};


SimulationRPC::SimulationRPC(ServerAsync& aServer)
 : mServer(aServer),
   mPageSize(sysconf(_SC_PAGESIZE)),
   mModelTimeStamp(0),
   mReceiver(nullptr),
   mInitComplete(false),
   mWaitForFirstTrigger(false)
{
   unsigned thisVersion = static_cast<unsigned>(floor(sqrt(ReleaseInfo::SCOPESERVER_VERSION)*1000+.5));
   mFirmwareVersion.mVersionMajor = thisVersion/1000000;
   mFirmwareVersion.mVersionMinor = (thisVersion/1000)%1000;
   mFirmwareVersion.mVersionPatch = thisVersion%1000;
   memset(&mLastReceivedModelResponse, 0, sizeof(mLastReceivedModelResponse));
   memset(&mLastReceivedLibraryVersion, 0, sizeof(mLastReceivedLibraryVersion));
   QByteArray rtbox3Value;
   QFile f("/sys/class/gpio/gpio497/value");
   if (f.open(QIODevice::ReadOnly))
   {
      rtbox3Value = f.readLine().trimmed();
      f.close();
   }
   mRtBox3 = (rtbox3Value == "1");
   mPerformanceCounter = new PerformanceCounter(this);
   if (openConnection() && checkConnection(false))
   {
      if (doQuerySimulation())
      {
         mModelTimeStamp = QDateTime::currentDateTime().toTime_t();
         mPerformanceCounter->init(mReceiver->getCpuPerformanceCounters(), mLastReceivedModelResponse.mCorePeriodTicks);
         syncDataBlockInfos();
      }
   }
   if (!QCoreApplication::arguments().contains("-nofan"))
   {
      mFanControl = new FanControl(this);
   }
}


SimulationRPC::~SimulationRPC()
{
   closeConnection();
}


bool SimulationRPC::openConnection()
{
   if (!mReceiver)
   {
      QThread* thread = new QThread;
      RPCReceiver* p = new RPCReceiver();
      p->moveToThread(thread);
      mReceiver = p;
      connect(p, SIGNAL (error(QString)), this, SLOT (receiveError(QString)));
      connect(thread, SIGNAL (started()), p, SLOT (process()));
      connect(p, SIGNAL (finished()), thread, SLOT (quit()));
      connect(p, SIGNAL (finished()), p, SLOT (deleteLater()));
      connect(thread, SIGNAL (finished()), thread, SLOT (deleteLater()));
      connect(p, &RPCReceiver::initComplete, this, &SimulationRPC::initComplete);
      connect(p, &RPCReceiver::scopeArmResponse, this, &SimulationRPC::scopeArmResponse);
      connect(p, &RPCReceiver::tuneParameterResponse, this, &SimulationRPC::tuneParameterResponse);
      connect(p, &RPCReceiver::sigLog, this, &SimulationRPC::log);
      connect(p, &RPCReceiver::sigError, this, &SimulationRPC::reportError);
      connect(p, &RPCReceiver::logMessage, this, &SimulationRPC::logMessageForWeb);
      connect(p, &RPCReceiver::logMessage, this, &SimulationRPC::logMessage);
      connect(this, &SimulationRPC::sendRequest, p, &RPCReceiver::send);
      connect(this, &SimulationRPC::shutdownRequest, p, &RPCReceiver::shutdown);
      mMessageBuffer.clear();
      QEventLoop eventLoop;
      connect(thread, &QThread::started, &eventLoop, &QEventLoop::quit);
      thread->start(QThread::HighPriority);
      eventLoop.exec();
      if (!mReceiver)
         return false;
   }
   return true;
}

bool SimulationRPC::checkConnection(bool aVerbose)
{
   if (!checkRunning())
   {
      if (aVerbose)
         mServer.reportError(tr("Realtime simulation has not been started."));
      return false;
   }
   if (!verifyAPIVersion(aVerbose))
   {
      return false;
   }
   return true;
}

bool SimulationRPC::closeConnection()
{
   if (mReceiver)
   {
      QEventLoop loop;
      connect(mReceiver.data(), &RPCReceiver::destroyed, &loop, &QEventLoop::quit);
      mReceiver->deleteLater();
      mReceiver = nullptr;
      loop.exec();
   }
   mDataCaptureMap.clear();
   mProgrammableValueConstMap.clear();
   return true;
}


void SimulationRPC::receiveError(QString aError)
{
   mServer.reportError(aError);
   closeConnection();
}


bool SimulationRPC::doQuerySimulation()
{
   int msg = MSG_QUERY_MODEL;
   QByteArray buffer;
   if (mReceiver->waitForMessage((char*)&msg, sizeof(msg), RSP_QUERY_MODEL, buffer, 1000))
   {
      if (buffer.size() < (int)sizeof(struct QueryModelResponse))
      {
         mServer.reportError(tr("No answer from simulation core."));
         closeConnection();
         return false;
      }
      struct QueryModelResponse* resp = (struct QueryModelResponse*)buffer.data();
      int offset = sizeof(struct QueryModelResponse);
      mLastReceivedModelResponse = *resp;
      mLastReceivedModelChecksum = buffer.mid(offset, mLastReceivedModelResponse.mChecksumLength);
      offset += mLastReceivedModelResponse.mChecksumLength;

      if (buffer.size() < offset + (int)sizeof(int))
      {
         mServer.reportError(tr("Cannot read model name length from core."));
         closeConnection();
         return false;
      }
      int modelNameLength = *((int*)buffer.mid(offset, sizeof(int)).data());
      offset += sizeof(int);
      if (buffer.size() < offset + modelNameLength)
      {
         mServer.reportError(tr("Cannot read model name from core."));
         closeConnection();
         return false;
      }
      mLastReceivedModelName = buffer.mid(offset, modelNameLength);
      // log(QString("Received Query Model Response, checksum ") + mLastReceivedModelChecksum);
      if (!mReceiver->mapBuffers(mLastReceivedModelResponse.mScopeBufferSize,
                                 mLastReceivedModelResponse.mRxTxBufferSize))
      {
         mServer.reportError(tr("Cannot map scope buffers."));
         closeConnection();
         return false;
      }
   }
   else
      return false;

   return true;
}


bool SimulationRPC::querySimulation(double& aSampleTime, int& aNumScopeSignals, 
                                    int& aNumTuneableParameters, 
                                    struct VersionType& aLibraryVersion, 
                                    QByteArray& aChecksum, QByteArray& aModelName)
{
   if (mLastReceivedModelResponse.mSampleTime == 0.0) // no response received yet
      return false;
   aSampleTime = mLastReceivedModelResponse.mSampleTime;
   aNumScopeSignals = mLastReceivedModelResponse.mNumScopeSignals;
   aNumTuneableParameters = mLastReceivedModelResponse.mNumTuneableParameters;
   aLibraryVersion = mLastReceivedLibraryVersion;
   aChecksum = mLastReceivedModelChecksum;
   aModelName = mLastReceivedModelName;
   return true;
}


bool SimulationRPC::armScope(const struct SimulationRPC::ArmMessage& aMsg)
{
   static QTime lastArmTime = QTime::currentTime().addSecs(-1);
   QTime currentTime = QTime::currentTime();
   if (lastArmTime.msecsTo(currentTime) < 20)
      return false;
   lastArmTime = currentTime;
   // mNotifier->setEnabled(true);
   emit sendRequest(QByteArray((char*)&aMsg, aMsg.mMsgLength));
   return true;
}


bool SimulationRPC::sendRawMessage(const QByteArray &aMessage)
{
   QByteArray msg(8, 0);
   msg.append(aMessage);
   struct MsgHdr* hdr = (struct MsgHdr*)msg.data();
   hdr->mMsg = MSG_RAW;
   hdr->mMsgLength = msg.length();
   emit sendRequest(aMessage);
   return true;
}


bool SimulationRPC::tuneParameters(QByteArray& aParameters)
{
   *(int*)aParameters.data() = MSG_TUNE_PARAMETERS;
   *(int*)(aParameters.data() + sizeof(int)) = aParameters.size();
   /*
   log(QString("Tune length = %1, %2 %3 %4").arg(aParameters.size())
      .arg(*(int*)aParameters.data())
      .arg(*(int*)(aParameters.data() + 4))
      .arg(*(double*)(aParameters.data() + 8)));
   */
   emit sendRequest(aParameters);
   return true;
}


bool SimulationRPC::getScopeBuffer(QVector<float> &aBuffer, int aBufferIndex, int aOffset)
{
   size_t len = aBuffer.size() * sizeof(float);
   char* buf = (char*)mReceiver->getScopeBuffer() + aBufferIndex * (mLastReceivedModelResponse.mScopeBufferSize / 2);
   memcpy(aBuffer.data(), buf + aOffset, len - aOffset);
   memcpy(aBuffer.data() + (len-aOffset)/sizeof(float), buf, aOffset);
   return true;
}


void SimulationRPC::log(QString aMsg)
{
   mServer.doLogging(aMsg);
}


void SimulationRPC::reportError(QString aMsg)
{
   mServer.reportError(aMsg);
   static const int msg[2] = { MSG_ERROR, 0 };
   QByteArray buf((char*)msg, sizeof(msg));
   buf.append(aMsg.toLocal8Bit());
   struct MsgHdr* hdr = (struct MsgHdr*)buf.data();
   hdr->mMsgLength = buf.size();
   emit sendRequest(buf);
}

void SimulationRPC::shutdownSimulation()
{
   emit simulationAboutToStop();

   if (checkConnection(false))
   {
      mServer.closeConnection();
      emit shutdownRequest();
      closeConnection();
   }
   mPerformanceCounter->deinit();
   // Wait gracefully for 1 second
   QEventLoop loop;
   QTimer::singleShot(1000, &loop, &QEventLoop::quit);
   loop.exec();
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "shutdown" << "1");
   mModelTimeStamp = 0;
}


QString SimulationRPC::startSimulation(bool aWaitForFirstTrigger)
{
   emit simulationStartRequested();

   if (checkRunning())
      return QString("Simulation is already running.");
   if (!QFile::exists("/lib/firmware/firmware"))
      return QString("No simulation executable loaded.");
   mLastReceivedModelName = QByteArray();
   memset(&mLastReceivedModelResponse, 0, sizeof(mLastReceivedModelResponse));
   memset(&mLastReceivedLibraryVersion, 0, sizeof(mLastReceivedLibraryVersion));
   QByteArray inputConfig(32, 0);
   //initDigitalInputs(inputConfig);
   mDataCaptureMap.clear();
   mProgrammableValueConstMap.clear();
   mInitComplete = false;
   mWaitForFirstTrigger = aWaitForFirstTrigger;
   if (!openConnection())
   {
      return QString("Cannot initialize communication.");
   }
   QEventLoop loop;
   connect(this, &SimulationRPC::initCompleteDone, &loop, &QEventLoop::quit);
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "load" << "1" << "/lib/firmware/firmware");
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "start" << "1");
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "shutdown" << "1");
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "load" << "1" << "/lib/firmware/firmware");
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "start" << "1");
   mModelTimeStamp = QDateTime::currentDateTime().toTime_t();

   QTimer::singleShot(2000, &loop, &QEventLoop::quit);
   if (!mInitComplete)
   {
      // Wait gracefully for 2 seconds until model startup is completed
      loop.exec();
   }
   if (mInitComplete && checkConnection(false))
   {
      doQuerySimulation();
      if ((mFirmwareVersion.mVersionMajor != mLastReceivedLibraryVersion.mVersionMajor) ||
          (mFirmwareVersion.mVersionMinor != mLastReceivedLibraryVersion.mVersionMinor))
      {
         return(QString("The application uses target support library version %1.%2 "
                        "but the firmware on this box only supports version %3.%4.")
                   .arg(mLastReceivedLibraryVersion.mVersionMajor)
                   .arg(mLastReceivedLibraryVersion.mVersionMinor)
                   .arg(mFirmwareVersion.mVersionMajor)
                   .arg(mFirmwareVersion.mVersionMinor));
      }
      mPerformanceCounter->init(mReceiver->getCpuPerformanceCounters(), mLastReceivedModelResponse.mCorePeriodTicks);
      syncDataBlockInfos();

      const int msg[3] = { MSG_START_SIMULATION, 0, mWaitForFirstTrigger};
      QByteArray buf((char*)msg, sizeof(msg));
      struct MsgHdr* hdr = (struct MsgHdr*)buf.data();
      hdr->mMsgLength = buf.size();
      emit sendRequest(buf);
   }
   else
      return QString("Communication with realtime simulation failed.");

   emit simulationStarted();

   return QString();
}


bool SimulationRPC::checkRunning()
{
   QFile stateFile("/sys/devices/jailhouse/cells/1/state");
   if (stateFile.exists())
   {
      stateFile.open(QIODevice::ReadOnly);
      QByteArray state = stateFile.readAll();
      stateFile.close();
      return state.startsWith("running");
   }
   return false;
}


quint32 SimulationRPC::peek(quint32 aAddress)
{
   QFile sharedMemoryFile("/dev/mem");
   sharedMemoryFile.open(QIODevice::ReadOnly | QIODevice::Unbuffered);
   quint32 pageAddr=(aAddress & ~(mPageSize-1));
   quint32 pageOffset=aAddress-pageAddr;

   uchar* sharedMemory = sharedMemoryFile.map(pageAddr, mPageSize, QFileDevice::MapPrivateOption);
   sharedMemoryFile.close();
   if (sharedMemory == NULL) 
   {
      log("Can't map memory");
      return 0;
   }
   quint32 ret = *((quint32 *)(sharedMemory+pageOffset));
   sharedMemoryFile.unmap(sharedMemory);
   return ret;
}


void SimulationRPC::poke(quint32 aAddress, quint32 aValue)
{
   QFile sharedMemoryFile("/dev/mem");
   sharedMemoryFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered);
   quint32 pageAddr=(aAddress & ~(mPageSize-1));
   quint32 pageOffset=aAddress-pageAddr;

   uchar* sharedMemory = sharedMemoryFile.map(pageAddr, mPageSize, QFileDevice::NoOptions);
   sharedMemoryFile.close();
   if (sharedMemory == NULL) 
   {
      log("Poke: can't map memory");
      return;
   }
   *((quint32 *)(sharedMemory+pageOffset)) = aValue;
   sharedMemoryFile.unmap(sharedMemory);
}


void SimulationRPC::initDigitalInputs(const QByteArray& aConfig)
{
   if (!QFile::exists("/sys/bus/i2c/devices/0-0074/gpio"))
      return; // V1.0 boxes
   if (aConfig.size() != 32)
   {
      log("initDigitalInputs(): invalid size.");
      return;
   }
   QDir ioDir("/sys/bus/i2c/devices/0-0074/gpio");
   QStringList chips = ioDir.entryList(QStringList() << "gpiochip*");
   if (chips.isEmpty())
      return;
   int baseAddr = chips[0].mid(8).toInt();
   for (int i=0; i<32; i++)
   {
      QString dirName = QString("/sys/class/gpio/gpio%1").arg(i+baseAddr);
      QFile directionFile(dirName + "/direction");
      directionFile.open(QIODevice::WriteOnly);
      directionFile.write("out\n");
      directionFile.close();
      QFile valueFile(dirName + "/value");
      valueFile.open(QIODevice::WriteOnly);
      if (aConfig[i] == 3)
         valueFile.write("1\n");
      else
         valueFile.write("0\n");
      valueFile.close();
      directionFile.open(QIODevice::WriteOnly);
      if (aConfig[i] != 2 && aConfig[i] != 3)
         directionFile.write("in\n");
      directionFile.close();
   }
}


void SimulationRPC::resetPerformanceCounter()
{
   mPerformanceCounter->resetMax();
}


bool SimulationRPC::verifyAPIVersion(bool aVerbose)
{
   struct QueryAPIMsg msg;
   msg.msg = MSG_QUERY_API_VERSION; 
   static const unsigned thisVersion = static_cast<unsigned>(floor(sqrt(ReleaseInfo::SCOPESERVER_VERSION)*1000+.5));
   msg.mFirmwareVersion.mVersionMajor = thisVersion/1000000;
   msg.mFirmwareVersion.mVersionMinor = (thisVersion/1000)%1000;
   msg.mFirmwareVersion.mVersionPatch = thisVersion%1000;

   int remoteApiVersion = 0;
   QByteArray buffer;
   if (mReceiver->waitForMessage((char*)&msg, sizeof(msg), RSP_QUERY_API_VERSION, buffer, 3000))
   {
      if (buffer.size() < (int)sizeof(remoteApiVersion) + (int)sizeof(mLastReceivedLibraryVersion))
      {
         log(QString("Cannot read API version."));
         return false;
      }
      remoteApiVersion = *((int*)buffer.data());
      mLastReceivedLibraryVersion = *((struct VersionType*)(buffer.data() + sizeof(int)));
   }
   else
   {
      log(QString("Timeout while waiting for API version."));
      return false;
   }
   if (remoteApiVersion != MY_API_VERSION)
   {
      if (aVerbose)
         mServer.reportError(tr("Wrong API version: expected %1, found %2.")
            .arg(MY_API_VERSION).arg(remoteApiVersion));
      return false;
   }
   return true;
}


bool SimulationRPC::syncDataBlockInfos()
{
   struct NumDataCapturesRequest
   {
      int msg;
      int mLength;
   };

   const struct NumDataCapturesRequest numReq = {
      .msg = MSG_GET_NUM_DATA_BLOCKS,
      .mLength = sizeof(struct NumDataCapturesRequest)
   };

   QByteArray buffer;
   if (!mReceiver)
      return false;
   if (!mReceiver->waitForMessage((char*)&numReq, sizeof(numReq), RSP_GET_NUM_DATA_BLOCKS, buffer, 1000))
      return false;
   else
   {
      if (buffer.size() < 2*(int)sizeof(int))
      {
         log(QString("Cannot read number of Data Capture blocks."));
         return false;
      }
      int numDataCaptures = *((int*)buffer.data());
      int numProgrammableValueConst = *((int*)(buffer.data() + sizeof(int)));
      for (int i=0; i<numDataCaptures; i++)
      {
         struct DataCaptureInfoRequest
         {
            int msg;
            int mLength;
            int mInstance;
         };

         const struct DataCaptureInfoRequest infoReq = {
            .msg = MSG_GET_DATA_CAPTURE_INFO,
            .mLength = sizeof(struct DataCaptureInfoRequest),
            .mInstance = i
         };
         if (mReceiver->waitForMessage((char*)&infoReq, sizeof(infoReq), RSP_GET_DATA_CAPTURE_INFO, buffer, 1000))
         {
            struct DataCaptureInfoResponse
            {
               int mInstance;
               int mWidth;
               int mNumSamples;
               int mComponentPathLength;
            };

            struct DataCaptureInfoResponse rsp;
            if (buffer.size() < (int)sizeof(struct DataCaptureInfoResponse))
            {
               log(QString("Cannot read data sink info."));
               return false;
            }
            rsp = *((struct DataCaptureInfoResponse*) buffer.data());
            DataCaptureInfo entry =
            {
               .mInstance = rsp.mInstance,
               .mWidth = rsp.mWidth,
               .mNumSamples = rsp.mNumSamples
            };
            volatile char* componentPath = (volatile char*)mReceiver->getRxBuffer();
            //componentPath[rsp.mComponentPathLength-1] = 0;
            const QString componentPathStr((const char*)componentPath);
            log(QString("Registering Data Capture %1").arg(componentPathStr));
            mDataCaptureMap[componentPathStr] = entry;
         }
      }

      for (int i=0; i<numProgrammableValueConst; i++)
      {
         struct ProgrammableValueConstInfoRequest
         {
            int msg;
            int mLength;
            int mInstance;
         };

         const struct ProgrammableValueConstInfoRequest infoReq = {
            .msg = MSG_GET_PROGRAMMABLE_VALUE_INFO,
            .mLength = sizeof(struct ProgrammableValueConstInfoRequest),
            .mInstance = i
         };
         if (mReceiver->waitForMessage((char*)&infoReq, sizeof(infoReq), RSP_GET_PROGRAMMABLE_VALUE_INFO, buffer, 1000))
         {
            struct ProgrammableValueConstInfoResponse
            {
               int mInstance;
               int mWidth;
               int mComponentPathLength;
            };

            struct ProgrammableValueConstInfoResponse rsp;
            if (buffer.size() < (int)sizeof(struct ProgrammableValueConstInfoResponse))
            {
               log(QString("Cannot read Programmable Value info."));
               return false;
            }
            rsp = *((struct ProgrammableValueConstInfoResponse*)buffer.data());
            ProgrammableValueConstInfo entry =
            {
               .mInstance = rsp.mInstance,
               .mWidth = rsp.mWidth,
            };
            volatile char* componentPath = (volatile char*)mReceiver->getRxBuffer();
            //componentPath[rsp.mComponentPathLength-1] = 0;
            const QString componentPathStr((const char*)componentPath);
            log(QString("Registering Programmable Value %1").arg(componentPathStr));
            mProgrammableValueConstMap[componentPathStr] = entry;
         }
      }
   }
   return true;
}


int SimulationRPC::getDataCaptureTriggerCount(const QString& aDataCapturePath)
{
   struct DataCaptureBufferFullCountRequest
   {
      int msg;
      int mLength;
      unsigned int mInstance;
   } req;
   req.msg = MSG_GET_DATA_CAPTURE_BUFFER_FULL_COUNT;
   QMap<QString, DataCaptureInfo>::const_iterator it = mDataCaptureMap.find(aDataCapturePath);
   if (it == mDataCaptureMap.end())
   {
      throw RTBoxError(tr("Invalid Data Capture path."));
   }
   req.mInstance = it->mInstance;
   req.mLength = sizeof(struct DataCaptureBufferFullCountRequest);
   QByteArray buffer;
   if (mReceiver->waitForMessage((char*)&req, sizeof(req), RSP_GET_DATA_CAPTURE_BUFFER_FULL_COUNT, buffer, 1000))
   {
      if (buffer.size() < (int)sizeof(int))
      {
         throw RTBoxError(tr("Cannot read Data Capture buffer full count."));
      }
      int bufferFullCount = *((int*) buffer.data());
      return bufferFullCount;
   }
   else
   {
      throw(RTBoxError(tr("Timeout waiting for Data Capture trigger count.")));
   }
}


void SimulationRPC::getDataCaptureData(const QString& aDataCapturePath,
                                    QVariantList& aData,
                                    int& aTriggerCount)
{

   struct DataCaptureDataRequest
   {
      int msg;
      int mLength;
      unsigned int mInstance;
   } req;
   aData.clear();
   req.msg = MSG_GET_DATA_CAPTURE_DATA;
   QMap<QString, DataCaptureInfo>::const_iterator it = mDataCaptureMap.find(aDataCapturePath);
   if (it == mDataCaptureMap.end())
   {
      throw RTBoxError(tr("Invalid Data Capture path."));
   }
   req.mInstance = it->mInstance;
   req.mLength = sizeof(struct DataCaptureDataRequest);
   QByteArray buffer;
   if (mReceiver->waitForMessage((char*)&req, sizeof(req), RSP_GET_DATA_CAPTURE_DATA, buffer, 1000))
   {
      if (buffer.size() < (int)sizeof(int))
      {
         throw RTBoxError(tr("Cannot read Data Capture buffer full count."));
      }
      aTriggerCount = *((int*)buffer.data());
      if (aTriggerCount)
      {
         int maxSamples = qMin((size_t)it->mNumSamples, mLastReceivedModelResponse.mRxTxBufferSize/(2 * sizeof(double) * it->mWidth));
         for (int i=0; i<maxSamples; i++)
         {
            QVariantList d;
            for (int j=0; j<it->mWidth; j++)
            {
               d << *((double*)mReceiver->getRxBuffer() + i*it->mWidth + j);
            }
            aData << (QVariant)d;
         }
      }
   }
   else
   {
      throw(RTBoxError(tr("Timeout waiting for Data Capture data.")));
   }
}


void SimulationRPC::setProgrammableValueData(const QString& aDataCapturePath, const QVariant& aData)
{
   struct ProgrammableValueConstDataRequest
   {
      int msg;
      int mLength;
      unsigned int mInstance;
   };
   struct ProgrammableValueConstDataRequest req = {
      .msg = MSG_SET_PROGRAMMABLE_VALUE_DATA,
      .mLength = sizeof(struct ProgrammableValueConstDataRequest),
      .mInstance = 0
   };
   QMap<QString, ProgrammableValueConstInfo>::const_iterator it = mProgrammableValueConstMap.find(aDataCapturePath);
   if (it == mProgrammableValueConstMap.end())
   {
      throw RTBoxError(tr("Invalid Programmable Value path."));
   }
   req.mInstance = it->mInstance;
   if (aData.type() == QVariant::Int || aData.type() == QVariant::Double)
   {
      if (it->mWidth != 1)
         throw(RTBoxError(tr("Wrong length of data parameter: expected %1 values, got scalar.")
                          .arg(it->mWidth)));

      *((double*)mReceiver->getTxBuffer()) = aData.toDouble();
   }
   else if (aData.type() == QVariant::List)
   {
      QVariantList val = aData.toList();
      if (val.size() != it->mWidth)
         throw(RTBoxError(tr("Wrong length of data parameter: expected %1 values, got %2.")
                          .arg(it->mWidth)
                          .arg(val.size())));
      if (val.size() > mLastReceivedModelResponse.mRxTxBufferSize/(2 * (int)sizeof(double)))
         throw(RTBoxError(tr("Too many list elements. Maximum allowed list size is %1.").arg(
                             mLastReceivedModelResponse.mRxTxBufferSize/(2 * sizeof(double)))));
      for (int i=0; i < it->mWidth; i++)
      {
         *((double*)mReceiver->getTxBuffer() + i) = val[i].toDouble();
      }
   }
   else
      throw(RTBoxError(tr("Wrong parameter type for method.")));
   QByteArray buffer;
   if (!mReceiver->waitForMessage((char*)&req, sizeof(req), RSP_SET_PROGRAMMABLE_VALUE_DATA, buffer, 1000))
   {
      throw(RTBoxError(tr("Timeout waiting for Programmable Value response.")));
   }
}


QStringList SimulationRPC::getDataCaptureBlocks() const
{
   return mDataCaptureMap.keys();
}


QStringList SimulationRPC::getProgrammableValueBlocks() const
{
   return mProgrammableValueConstMap.keys();
}


void SimulationRPC::scopeArmResponse(QByteArray aData)
{
   const struct ArmResponse* resp = (const struct ArmResponse*)aData.data();
   if (resp->mNumActiveSignals <= 0 || resp->mLength <= 0)
      return;
   // sanity check
   if ((aData.size() != sizeof(struct ArmResponse) + (resp->mNumActiveSignals-2) * sizeof(qint16)) ||
        (resp->mLength % (resp->mNumActiveSignals * sizeof(float)) != 0))
   {
      //log(QString("%1").arg(sizeof(struct ArmResponse) + (resp->mNumActiveSignals-1) * sizeof(qint16)));
      log(QString("Corrupt scopeArmResponse received. l=%1").arg(aData.size()));
      return;
   }
   /*
   QByteArray l;
   for (int i=0; i<resp->mNumActiveSignals; i++)
      l.append(QByteArray::number(*(&resp->mActiveSignals+i)));
   qDebug() << "Received" << l.data();
   */
   mServer.receiveScopeData(*resp);
}


void SimulationRPC::tuneParameterResponse(int aErrorCode)
{
   mServer.receiveTuneParametersResponse(aErrorCode);
}


bool SimulationRPC::syncDigitalInputConfig()
{
   struct DigitalInputConfigRequest
   {
      int msg;
      int mLength;
   };

   const struct DigitalInputConfigRequest configReq = {
      .msg = MSG_QUERY_DI_CONFIG,
      .mLength = sizeof(struct DigitalInputConfigRequest)
   };
   QByteArray buffer;
   if (!mReceiver)
      return false;
   if (!mReceiver->waitForMessage((char*)&configReq, sizeof(configReq), RSP_QUERY_DI_CONFIG, buffer, 1000))
      return false;
   else
   {
      if (buffer.size() != 32)
         return false;
      initDigitalInputs(buffer);
   }

   return true;
}

void SimulationRPC::logMessageForWeb(int aFlags, QByteArray aMessage)
{
   Q_UNUSED(aFlags);
   mMessageBuffer += aMessage;
}

void SimulationRPC::initComplete()
{
   mInitComplete = true;
   emit initCompleteDone();
}

