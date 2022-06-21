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
#include "TemperatureControl.h"
#include <unistd.h>
#include <cmath>
#include <sys/mman.h>
#include <sys/time.h>
#include "PerformanceCounter.h"
#include "xml-rpc/xparameters.h"

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
   mWaitForFirstTrigger(false),
   mSimulationStatus(SimulationStatus::STOPPED),
   mLastScopeArmTime(std::chrono::steady_clock::now()),
   mHwVersionMajor(0),
   mHwVersionMinor(0),
   mHwVersionRevision(0)
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
   QFile eeprom("/sys/bus/i2c/devices/0-0052/eeprom");
   if (eeprom.open(QIODevice::ReadOnly))
   {
       uint16_t version;
       eeprom.read((char*)&version, sizeof(version));
       if (version == 1)
       {
          eeprom.read((char*)&mHwVersionMajor, sizeof(mHwVersionMajor));
          eeprom.read((char*)&mHwVersionMinor, sizeof(mHwVersionMinor));
          eeprom.read((char*)&mHwVersionRevision, sizeof(mHwVersionRevision));
       }
       eeprom.close();
   }

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

   bool controlFan = !QCoreApplication::arguments().contains("-nofan");
   mTemperatureControl = new TemperatureControl(this, controlFan);
   connect(mTemperatureControl, &TemperatureControl::displayMessageRequest, this, &SimulationRPC::logMessage);
   connect(mTemperatureControl, &TemperatureControl::displayMessageRequest, this, &SimulationRPC::logMessageForWeb);
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
      connect(p, &RPCReceiver::initEthercat, this, &SimulationRPC::initEthercat);
      connect(p, &RPCReceiver::scopeArmResponse, this, &SimulationRPC::scopeArmResponse);
      connect(p, &RPCReceiver::tuneParameterResponse, this, &SimulationRPC::tuneParameterResponse);
      connect(p, &RPCReceiver::sigLog, this, &SimulationRPC::log);
      connect(p, &RPCReceiver::sigError, this, &SimulationRPC::reportError);
      connect(p, &RPCReceiver::logMessage, this, &SimulationRPC::logMessageForWeb);
      connect(p, &RPCReceiver::logMessage, this, &SimulationRPC::logMessage);
      connect(p, &RPCReceiver::simulationError, this, &SimulationRPC::simulationError);
      connect(this, &SimulationRPC::sendRequest, p, &RPCReceiver::send);
      connect(this, &SimulationRPC::shutdownRequest, p, &RPCReceiver::shutdown);
      connect(this, &SimulationRPC::initToFileHandler, p, &RPCReceiver::initializeToFileHandler);
      mMessageBuffer.clear();

      QEventLoop eventLoop;
      connect(thread, &QThread::started, &eventLoop, &QEventLoop::quit);
      thread->start(QThread::TimeCriticalPriority);
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
                                 mLastReceivedModelResponse.mToFileBufferSize,
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
                                    QByteArray& aChecksum, QByteArray& aModelName,
                                    int& aAnalogOutputVoltageRange, int& aAnalogInputVoltageRange,
                                    int& aDigitalOutputVoltage)
{
   if (mLastReceivedModelResponse.mSampleTime == 0.0) // no response received yet
      return false;
   aSampleTime = mLastReceivedModelResponse.mSampleTime;
   aNumScopeSignals = mLastReceivedModelResponse.mNumScopeSignals;
   aNumTuneableParameters = mLastReceivedModelResponse.mNumTuneableParameters;
   aLibraryVersion = mLastReceivedLibraryVersion;
   aChecksum = mLastReceivedModelChecksum;
   aModelName = mLastReceivedModelName;
   aAnalogOutputVoltageRange = mLastReceivedModelResponse.mDacSpan;
   aAnalogInputVoltageRange = mLastReceivedModelResponse.mAnalogInputVoltageRange;
   aDigitalOutputVoltage = mLastReceivedModelResponse.mDigitalOutVoltage;
   return true;
}


bool SimulationRPC::armScope(const struct SimulationRPC::ArmMessage& aMsg)
{
   static constexpr std::chrono::milliseconds threshold(20);
   const auto currentTime = std::chrono::steady_clock::now();
   if (currentTime - mLastScopeArmTime < threshold)
      return false;
   mLastScopeArmTime = currentTime;
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


bool SimulationRPC::getScopeBuffer(uint8_t* aData, int aLength, int aBufferIndex, int aOffset)
{
   uint8_t* buf = (uint8_t*)mReceiver->getScopeBuffer() + aBufferIndex * (mLastReceivedModelResponse.mScopeBufferSize / 2);
   memcpy(aData, buf + aOffset, aLength - aOffset);
   memcpy(aData + aLength-aOffset, buf, aOffset);
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
   mSimulationStatus = SimulationStatus::ERROR;
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
   mSimulationStatus = SimulationStatus::STOPPED;
   // Wait gracefully for 1 second
   QEventLoop loop;
   QTimer::singleShot(800, &loop, &QEventLoop::quit);
   loop.exec();
   if (checkRunning())
   {
      // disable Powerstages
      uint32_t reg = peek(XPAR_DIGITALOUTPUT_POWERSTAGEPROTECTION_0_S00_AXI_BASEADDR);
      poke(XPAR_DIGITALOUTPUT_POWERSTAGEPROTECTION_0_S00_AXI_BASEADDR, reg & 0x3fffffff);
      // stop timer unconditionally
      poke(XPAR_TRIGGERMANAGER_0_0, 0);
      QTimer::singleShot(200, &loop, &QEventLoop::quit);
      loop.exec();
   }
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "shutdown" << "1");
   mModelTimeStamp = 0;
   mSimulationStatus = SimulationStatus::STOPPED;
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
   mDataCaptureMap.clear();
   mProgrammableValueConstMap.clear();
   mInitComplete = false;
   mWaitForFirstTrigger = aWaitForFirstTrigger;
   mSimulationStatus = SimulationStatus::STOPPED;
   mMessageBuffer.clear();

   if (!openConnection())
   {
      return QString("Cannot initialize communication.");
   }
   QEventLoop loop;
   connect(this, &SimulationRPC::initCompleteDone, &loop, &QEventLoop::quit);
   connect(mReceiver, &RPCReceiver::simulationError, &loop, &QEventLoop::quit);
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "load" << "1" << "/lib/firmware/firmware");
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "start" << "1");
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "shutdown" << "1");
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "load" << "1" << "/lib/firmware/firmware");
   QProcess::execute("/usr/sbin/jailhouse", QStringList() << "cell" << "start" << "1");
   mModelTimeStamp = QDateTime::currentDateTime().toTime_t();

   QTimer::singleShot(2000, &loop, &QEventLoop::quit);
   if (!mInitComplete && mSimulationStatus != SimulationStatus::ERROR)
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

      QByteArray buf;
      buf.resize(sizeof(struct SimulationStartMsg));
      struct SimulationStartMsg* msg = (struct SimulationStartMsg*)buf.data();
      msg->mMsg = MSG_START_SIMULATION;
      msg->mMsgLength = buf.size();
      msg->mStartOnFirstTrigger = mWaitForFirstTrigger;
      struct timeval tv;
      gettimeofday(&tv, NULL);
      msg->mStartSec = tv.tv_sec;
      msg->mStartUSec = tv.tv_usec;
      msg->mHwVersionMajor = mHwVersionMajor;
      msg->mHwVersionMinor = mHwVersionMinor;
      emit sendRequest(buf);
   }
   else if (mSimulationStatus == SimulationStatus::ERROR)
      return QString("Simulation startup failed.");
   else
      return QString("Communication with realtime simulation failed.");

   emit simulationStarted();
   mSimulationStatus = SimulationStatus::RUNNING;

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


bool SimulationRPC::setDigitalOut(int aGpio, bool aValue)
{
   QString dirName = QString("/sys/class/gpio/gpio%1").arg(aGpio);
   if (!QDir(dirName).exists())
   {
      QFile exportFile("/sys/class/gpio/export");
      exportFile.open(QIODevice::WriteOnly);
      exportFile.write(QByteArray::number(aGpio).append('\n'));
      exportFile.close();
   }
   if (!QDir(dirName).exists())
      return false;
   QFile directionFile(dirName + "/direction");
   directionFile.open(QIODevice::WriteOnly);
   directionFile.write("out\n");
   directionFile.close();
   QFile valueFile(dirName + "/value");
   valueFile.open(QIODevice::WriteOnly);
   valueFile.write(aValue ? "1\n" : "0\n");
   valueFile.close();
   return true;
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
      log(QString("Got API Response in time."));
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
      int numToFileBlocks = *((int*)(buffer.data() + 2 * sizeof(int)));
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
            struct DataCaptureInfoResponseV2
            {
               int mInstance;
               int mWidth;
               int mNumSamples;
               int mComponentPathLength;
               double mSampleTime;
            };

            struct DataCaptureInfoResponseV2* rsp;
            if (buffer.size() < (int)sizeof(struct DataCaptureInfoResponse))
            {
               log(QString("Cannot read data sink info."));
               return false;
            }
            rsp = (struct DataCaptureInfoResponseV2*) buffer.data();
            DataCaptureInfo entry =
            {
               .mInstance = rsp->mInstance,
               .mWidth = rsp->mWidth,
               .mNumSamples = rsp->mNumSamples
            };
            if (buffer.size() == (int)sizeof(struct DataCaptureInfoResponseV2))
               entry.mSampleTime = rsp->mSampleTime;
            else
               entry.mSampleTime = getSampleTime();
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
               .mWidth = rsp.mWidth
            };
            volatile char* componentPath = (volatile char*)mReceiver->getRxBuffer();
            //componentPath[rsp.mComponentPathLength-1] = 0;
            const QString componentPathStr((const char*)componentPath);
            log(QString("Registering Programmable Value %1").arg(componentPathStr));
            mProgrammableValueConstMap[componentPathStr] = entry;
         }
      }
      for (int i=0; i<numToFileBlocks; i++)
      {
         struct ToFileInfoRequest
         {
            int mMsg;
            int mLength;
            int mInstance;
         };

         const struct ToFileInfoRequest infoReq = {
            .mMsg = MSG_GET_TO_FILE_INFO,
            .mLength = sizeof(struct ToFileInfoRequest),
            .mInstance = i
         };
         if (mReceiver->waitForMessage((char*)&infoReq, sizeof(infoReq), RSP_GET_TO_FILE_INFO, buffer, 1000))
         {
            struct ToFileInfoResponse
            {
               int mInstance;
               int mFileType;
               int mWriteDevice;
               int mNumSamples;
               int mWidth;
               int mBufferOffset;
               int mFileNameLength;
            };

            struct ToFileInfoResponse rsp;
            if (buffer.size() < (int)sizeof(struct ToFileInfoResponse))
            {
               log(QString("Cannot read To File block info."));
               return false;
            }
            rsp = *((struct ToFileInfoResponse*)buffer.data());
            volatile char* fileName = (volatile char*)mReceiver->getToFileBuffer();
            const QString fileNameStr((const char*)fileName);
            log(QString("Registering ToFile block with file name: %1").arg(fileNameStr));
            emit initToFileHandler(fileNameStr, mLastReceivedModelName, rsp.mWidth, rsp.mNumSamples, rsp.mBufferOffset, rsp.mFileType, rsp.mWriteDevice);
         }
      }
   }
   return true;
}


int SimulationRPC::getDataCaptureTriggerCount(const QString& aDataCapturePath)
{
   if (mSimulationStatus != SimulationStatus::RUNNING)
      throw(RTBoxError(tr("Simulation is not running.")));

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
                                    int& aTriggerCount, double& aSampleTime)
{
   if (mSimulationStatus != SimulationStatus::RUNNING)
      throw(RTBoxError(tr("Simulation is not running.")));

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
      aSampleTime = it->mSampleTime;
   }
   else
   {
      throw(RTBoxError(tr("Timeout waiting for Data Capture data.")));
   }
}


void SimulationRPC::setProgrammableValueData(const QString& aDataCapturePath, const QVariant& aData)
{
   if (mSimulationStatus != SimulationStatus::RUNNING)
      throw(RTBoxError(tr("Simulation is not running.")));

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

void SimulationRPC::initEthercat(int aUseEthercat)
{
   QDir ioDir("/sys/bus/i2c/devices/0-0074/gpio");
   QStringList chips = ioDir.entryList(QStringList() << "gpiochip*");
   if (chips.isEmpty())
      return;
   int baseAddr = chips[0].mid(8).toInt();
   QFile binFile("/lib/firmware/esf_r5.elf");
   if (!binFile.exists())
      mServer.reportError(tr("Ethercat firmware file not found."));
   else
   {
      setDigitalOut(baseAddr+3, !aUseEthercat); // set ethercat multiplexer
      setDigitalOut(baseAddr+4, 0); // assert reset of ethercat chip
      setDigitalOut(baseAddr+4, 1); // deassert reset of ethercat chip
      QFile stateFile("/sys/class/remoteproc/remoteproc1/state");
      stateFile.open(QIODevice::ReadWrite);
      QByteArray state = stateFile.readAll();
      stateFile.reset();
      if (state.startsWith("running"))
      {
         stateFile.write("stop\n");
         do {
            state = stateFile.readAll();
            stateFile.reset();
         } while (!state.startsWith("offline"));
      }
      if (aUseEthercat)
      {
         QFile firmwareFile("/sys/class/remoteproc/remoteproc1/firmware");
         firmwareFile.open(QIODevice::WriteOnly);
         firmwareFile.write("esf_r5.elf\n");
         firmwareFile.close();
         stateFile.write("start\n");
      }
      stateFile.close();
   }
}


void SimulationRPC::simulationError()
{
   mSimulationStatus = SimulationStatus::ERROR;
}


