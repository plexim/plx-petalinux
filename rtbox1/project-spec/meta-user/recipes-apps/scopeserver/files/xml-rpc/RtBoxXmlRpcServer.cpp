/*
 * Copyright (c) 2017-2020 Plexim GmbH.
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

#include "RtBoxXmlRpcServer.h"

#include "maiaXmlRpcServer.h"
#include "maiaFault.h"
#include "RTBoxError.h"
#include "SimulationRPC.h"
#include "ReleaseInfo.h"
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>
#include <QtCore/QProcess>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <cmath>



RtBoxXmlRpcServer::RtBoxXmlRpcServer(SimulationRPC& aSimulation, int aPort, QObject* aParent)
 : QObject(aParent),
   mSimulation(aSimulation),
   mBoardSerial(0)
{
   mServer = new MaiaXmlRpcServer(aPort);
   mServer->addMethod("rtbox.queryCounter", this, "queryCounter",
                      "Returns the current performance counters.");
   mServer->addMethod("rtbox.resetCounter", this, "resetCounter",
                      "Resets the performance counters.");
   mServer->addMethod("rtbox.querySimulation", this, "querySimulation",
                      "Returns general information about the running simulation.");
   mServer->addMethod("rtbox.hostname", this, "hostname",
                      "Returns the hostname.");
   mServer->addMethod("rtbox.status", this, "status",
                      "Returns the current status of the running simulation.");
   mServer->addMethod("rtbox.start", this, "start",
                      "(Re)starts the realtime simulation.");
   mServer->addMethod("rtbox.stop", this, "stop",
                      "Stops the current realtime simulation.");
   mServer->addMethod("rtbox.serials", this, "serials",
                      "Returns hardware information.");
   mServer->addMethod("rtbox.getCaptureTriggerCount", this, "getDataCaptureTriggerCount",
                      "Returns how often a Data Capture block was triggered.");
   mServer->addMethod("rtbox.getCaptureData", this, "getDataCaptureData",
                      "Returns data from a Data Capture block.");
   mServer->addMethod("rtbox.load", this, "loadElf",
                      "Loads an executable simulation file on the RT-Box.");
   mServer->addMethod("rtbox.setProgrammableValue", this, "setProgrammableValueData",
                      "Sets data in a Programmable Value block.");
   mServer->addMethod("rtbox.getProgrammableValueBlocks", this, "getProgrammableValueBlocks",
                      "Gets a list of the Programmable Value blocks in the model.");
   mServer->addMethod("rtbox.getDataCaptureBlocks", this, "getDataCaptureBlocks",
                      "Gets a list of the Data Capture blocks in the model.");
   mServer->addMethod("rtbox.reboot", this, "reboot", "");

   QProcess eepromProc;
   eepromProc.start("eeprom_config", QStringList() << "-s");
   eepromProc.waitForFinished();
   mCpuSerial = eepromProc.readAllStandardOutput().trimmed().toInt();
   QFile eeprom("/sys/bus/i2c/devices/0-0052/eeprom");
   if (eeprom.open(QIODevice::ReadOnly))
   {
       unsigned short version;
       eeprom.read((char*)&version, sizeof(version));
       if (version == 1)
       {
          unsigned short hwMajor;
          unsigned short hwMinor;
          char rev;
          eeprom.read((char*)&hwMajor, sizeof(hwMajor));
          eeprom.read((char*)&hwMinor, sizeof(hwMinor));
          eeprom.read((char*)&rev, sizeof(rev));
          mBoardRevision = QString("%1.%2 Rev. %3").arg(hwMajor).arg(hwMinor).arg(rev);
          eeprom.seek(24);
          eeprom.read((char*)&mBoardSerial, sizeof(mBoardSerial));
       }
       eeprom.close();
   }
   QByteArray mac;
   readLineFile("/sys/devices/soc0/amba/e000b000.ethernet/net/eth0/address", mac);
   mMacAddress = mac;
   quint32 fpgaVersion = mSimulation.peek(0x41230000);
   mFpgaVersion = QString("%1.%2.%3")
      .arg(fpgaVersion >> 28).arg((fpgaVersion >> 12) & 0xffff).arg(fpgaVersion & 0xfff);
   QDir hwmonDir("/sys/bus/i2c/devices/0-0048/hwmon");
   QFileInfoList monEntries = hwmonDir.entryInfoList(QStringList() << "hwmon*");
   if (!monEntries.isEmpty())
      mFanFilename = monEntries[0].canonicalFilePath()+"/fan1_input";
}

RtBoxXmlRpcServer::~RtBoxXmlRpcServer()
{
   delete mServer;
}

QVariant RtBoxXmlRpcServer::queryCounter()
{
   QVariant ret;
   QVariantMap retValues;
   SimulationRPC::QueryPerformanceCounterResponse counter;
   if (mSimulation.readPerformanceCounter(counter))
   {
      retValues["maxCycleTime"] = counter.mMaxCycleTime;
      retValues["runningCycleTime"] = counter.mRunningCycleTime;
   }
   else
   {
      retValues.insert("maxCycleTime", "");
      retValues.insert("runningCycleTime", "");
   }
   retValues["modelTimeStamp"] = mSimulation.getModelTimeStamp();
   return retValues;
}

QVariant RtBoxXmlRpcServer::resetCounter()
{
   QVariant ret;
   mSimulation.resetPerformanceCounter();
   return ret;
}

QVariant RtBoxXmlRpcServer::hostname()
{
   return QHostInfo::localHostName();
}

QVariant RtBoxXmlRpcServer::querySimulation()
{
   QVariantMap retValues;

   double sampleTime = 0.0;
   int numScopeSignals;
   int numTuneableParameters;
   QByteArray checksum;
   QByteArray modelName;
   int analogOutVoltageRange;
   int analogInVoltageRange;
   int digitalOutVoltage;
   SimulationRPC::VersionType libraryVersion;
   if (mSimulation.querySimulation(sampleTime, numScopeSignals, 
                                   numTuneableParameters, libraryVersion, 
                                   checksum, modelName, analogOutVoltageRange,
                                   analogInVoltageRange, digitalOutVoltage))

   {
      retValues["sampleTime"] = sampleTime;
      retValues["modelName"] = QString(modelName);
      retValues["applicationVersion"] = QString::number(libraryVersion.mVersionMajor) + "." + 
                                        QString::number(libraryVersion.mVersionMinor) + "." +
                                        QString::number(libraryVersion.mVersionPatch);
      retValues["modelTimeStamp"] = mSimulation.getModelTimeStamp();
      retValues["analogOutVoltageRange"] = analogOutVoltageRange;
      retValues["analogInVoltageRange"] = analogInVoltageRange;
      retValues["digitalOutVoltage"] = digitalOutVoltage;
   }
   else
   {
      retValues.insert("sampleTime", "");
      retValues.insert("modelName", "");
      retValues.insert("applicationVersion", "");
      retValues.insert("modelTimeStamp", 0);
      retValues.insert("analogOutVoltageRange", 0);
      retValues.insert("analogInVoltageRange", 0);
      retValues.insert("digitalOutVoltage", 0);
   }

   QString status;
   switch(mSimulation.getStatus())
   {
   case SimulationRPC::SimulationStatus::RUNNING:
      status = "running";
      break;
   case SimulationRPC::SimulationStatus::ERROR:
      status = "error";
      break;
   case SimulationRPC::SimulationStatus::STOPPED:
   default:
      status = "stopped";
      break;
   }

   retValues["status"] = status;
   return retValues;
}

QVariant RtBoxXmlRpcServer::status(int aModelTimeStamp, int aLogPosition)
{
   QVariantMap retValues;
   int logPosition = 0;
   int clearLog = 1;
   if (aModelTimeStamp == mSimulation.getModelTimeStamp())
   {
      logPosition = aLogPosition;
      clearLog = 0;
   }
    QByteArray temp;
   readLineFile("/sys/devices/soc0/amba/f8007100.adc/iio:device0/in_temp0_raw", temp);
   QByteArray fanSpeed;
   readLineFile(mFanFilename, fanSpeed);
   QString log;
   QFile logFile("/sys/kernel/debug/remoteproc/remoteproc0/trace0");
   if (logFile.open(QIODevice::ReadOnly))
   {
      logFile.seek(logPosition);
      log = logFile.readAll();
      logPosition = logFile.pos();
      logFile.close();  
   }
   retValues["temperature"] = (temp.toUInt() * 503.975) / 4096 - 273.15;
   retValues["fanSpeed"] = fanSpeed.toInt();
   retValues["logPosition"] = logPosition;
   retValues["applicationLog"] = log;
   retValues["clearLog"] = clearLog;
   retValues["modelTimeStamp"] = mSimulation.getModelTimeStamp();
   return retValues;
}

QVariant RtBoxXmlRpcServer::start(int aStartOnFirstTrigger)
{
   QString errorString = mSimulation.startSimulation(aStartOnFirstTrigger);
   if (!errorString.isEmpty())
   {
      throw(RTBoxError(errorString));
   }
   return QVariant();
}

QVariant RtBoxXmlRpcServer::stop()
{
   mSimulation.shutdownSimulation();
   return QVariant();
}

QVariant RtBoxXmlRpcServer::serials()
{
   static const unsigned thisVersion = 
      static_cast<unsigned>(floor(sqrt (ReleaseInfo::SCOPESERVER_VERSION)*1000+.5));
   static const QString firmwareVersion = QString("%1.%2.%3")
   .arg(thisVersion/1000000).arg((thisVersion/1000)%1000).arg(thisVersion%1000);
   QVariantList ipAddresses;
   foreach (const QHostAddress &address, QNetworkInterface::allAddresses())
   {
      if (address.protocol() == QAbstractSocket::IPv4Protocol &&
          address != QHostAddress(QHostAddress::LocalHost))
         ipAddresses << address.toString();
   }
   QVariantMap retValues;
   retValues["cpu"] = mCpuSerial;
   retValues["mac"] = mMacAddress;
   retValues["boardRevision"] = mBoardRevision;
   retValues["firmwareVersion"] = firmwareVersion;   
   retValues["firmwareBuild"] = ReleaseInfo::revisionInfo;
   retValues["fpgaVersion"] = mFpgaVersion;
   retValues["ipAddresses"] = ipAddresses;
   if (mBoardSerial)
      retValues["board"] = mBoardSerial;
   retValues["rtboxType"] = mSimulation.isRTBoxCE() ? "PLECS RT Box CE" : "PLECS RT Box 1";
   return retValues;
}

QVariant RtBoxXmlRpcServer::getDataCaptureTriggerCount(const QString& aDataCapturePath)
{
   return mSimulation.getDataCaptureTriggerCount(aDataCapturePath);
}

QVariant RtBoxXmlRpcServer::getDataCaptureData(const QString& aDataCapturePath)
{
   QVariantList data;
   int triggerCount;
   double sampleTime;
   mSimulation.getDataCaptureData(aDataCapturePath, data, triggerCount, sampleTime);
   QVariantMap res;
   res["data"] = data;
   res["triggerCount"] = triggerCount;
   res["sampleTime"] = sampleTime;
   return res;
}

void RtBoxXmlRpcServer::readLineFile(const QString& aFilename, QByteArray& aContent)
{
   QFile f(aFilename);
   if (f.open(QIODevice::ReadOnly))
   {
      aContent = f.readLine().trimmed();
      f.close();
   }
   else
      aContent.clear();
}

QVariant RtBoxXmlRpcServer::loadElf(const QByteArray &aData)
{
   mSimulation.shutdownSimulation();
   QFile firmwareFile("/lib/firmware/firmware");
   if (firmwareFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered))
   {
      if (firmwareFile.write(aData) != aData.size())
      {
         throw(RTBoxError(QString("Cannot write to firmware file: %1").arg(firmwareFile.errorString())));
      }
      firmwareFile.close();
      return 0;
   }
   else
      throw(RTBoxError(QString("Cannot open firmware file: %1").arg(firmwareFile.errorString())));
}

QVariant RtBoxXmlRpcServer::setProgrammableValueData(const QString& aProgrammableValuePath,
                                              const QVariantList& aData)
{
   mSimulation.setProgrammableValueData(aProgrammableValuePath, aData);
   return QVariant();
}

QVariant RtBoxXmlRpcServer::setProgrammableValueData(const QString& aProgrammableValuePath, double aData)
{
   mSimulation.setProgrammableValueData(aProgrammableValuePath, aData);
   return QVariant();
}

QVariant RtBoxXmlRpcServer::setProgrammableValueData(const QString& aProgrammableValuePath, int aData)
{
   mSimulation.setProgrammableValueData(aProgrammableValuePath, aData);
   return QVariant();
}

QVariant RtBoxXmlRpcServer::getProgrammableValueBlocks()
{
   QStringList keys = mSimulation.getProgrammableValueBlocks();
   QVariantList res;
   foreach (const QString& key, keys)
      res << key;
   return res;
}

QVariant RtBoxXmlRpcServer::getDataCaptureBlocks()
{
   QStringList keys = mSimulation.getDataCaptureBlocks();
   QVariantList res;
   foreach (const QString& key, keys)
      res << key;
   return res;
}

QVariant RtBoxXmlRpcServer::reboot(const QString& aSecret)
{
   if (aSecret == "reboot")
   {
      QProcess::execute("/sbin/reboot");
   }
   return QVariant();
}

