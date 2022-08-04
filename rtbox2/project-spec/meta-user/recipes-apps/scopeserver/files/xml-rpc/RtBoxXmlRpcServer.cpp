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
#include <QtCore/QFileInfoList>
#include <cmath>
#include "xparameters.h"
#include "PerformanceCounter.h"

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
   readLineFile("/sys/devices/platform/amba/ff0b0000.ethernet/net/eth0/address", mac);
   mMacAddress = mac;
   quint32 fpgaVersion = mSimulation.peek(XPAR_VERSION_AXI_GPIO_0_BASEADDR);
   mFpgaVersion = QString("%1.%2.%3")
      .arg(fpgaVersion >> 28).arg((fpgaVersion >> 12) & 0xffff).arg(fpgaVersion & 0xfff);
   {
      QDir hwmonDir("/sys/bus/i2c/devices/0-0040/hwmon");
      QFileInfoList monEntry = hwmonDir.entryInfoList(QStringList() << "hwmon*");
      if (!monEntry.isEmpty())
      {
         mMeasureHwmon1VoltInput = monEntry[0].canonicalFilePath() + "/in%1_input";
         mMeasureHwmon1CurrInput = monEntry[0].canonicalFilePath() + "/curr%1_input";
      }
   }
   {
      QDir hwmonDir("/sys/bus/i2c/devices/0-0041/hwmon");
      QFileInfoList monEntry = hwmonDir.entryInfoList(QStringList() << "hwmon*");
      if (!monEntry.isEmpty())
      {
         mMeasureHwmon2VoltInput = monEntry[0].canonicalFilePath() + "/in1_input";
         mMeasureHwmon2CurrInput = monEntry[0].canonicalFilePath() + "/curr%1_input";
      }
   }
   {
      QDir hwmonDir("/sys/bus/i2c/devices/0-004c/hwmon");
      QFileInfoList monEntry = hwmonDir.entryInfoList(QStringList() << "hwmon*");
      if (!monEntry.isEmpty())
      {
         mMeasureHwmonBoardTemp = monEntry[0].canonicalFilePath() + "/temp1_input";
      }
   }
   mHasCPUFan = QFile("/sys/bus/i2c/devices/0-004b/hwmon/hwmon1/fan1_input").exists();
}

RtBoxXmlRpcServer::~RtBoxXmlRpcServer()
{
   delete mServer;
}

QVariant RtBoxXmlRpcServer::queryCounter()
{
   QVariant ret;
   QVariantMap retValues;

   const QPointer<PerformanceCounter>& counter = mSimulation.getPerformanceCounter();
   if (counter && counter->isValid())
   {
      retValues["maxCycleTime1"] = (int)counter->getAbsMax(0);
      retValues["runningCycleTime1"] = (int)counter->getRunningMax(0, 5);
      retValues["maxCycleTime2"] = (int)counter->getAbsMax(1);
      retValues["runningCycleTime2"] = (int)counter->getRunningMax(1, 5);
      retValues["maxCycleTime3"] = (int)counter->getAbsMax(2);
      retValues["runningCycleTime3"] = (int)counter->getRunningMax(2, 5);
   }
   else
   {
      retValues.insert("maxCycleTime1", "");
      retValues.insert("runningCycleTime1", "");
      retValues.insert("maxCycleTime2", "");
      retValues.insert("runningCycleTime2", "");
      retValues.insert("maxCycleTime3", "");
      retValues.insert("runningCycleTime3", "");
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
   QVariantList periodTicks;
   SimulationRPC::VersionType libraryVersion;
   int analogOutVoltageRange;
   int analogInVoltageRange;
   int digitalOutVoltage;

   const QPointer<PerformanceCounter>& counter = mSimulation.getPerformanceCounter();
   if (mSimulation.querySimulation(sampleTime, numScopeSignals, 
                                   numTuneableParameters, libraryVersion, 
                                   checksum, modelName, analogOutVoltageRange,
                                   analogInVoltageRange, digitalOutVoltage) &&
       counter)

   {
      retValues["sampleTime"] = sampleTime;
      retValues["modelName"] = QString(modelName);
      retValues["applicationVersion"] = QString::number(libraryVersion.mVersionMajor) + "." + 
                                        QString::number(libraryVersion.mVersionMinor) + "." +
                                        QString::number(libraryVersion.mVersionPatch);
      for (int i=0; i<3; i++)
      {
         periodTicks.append((int)counter->getPeriodTicks(i));
      }
      retValues["periodTicks"] = periodTicks;
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
      for (int i=0; i<3; i++)
      {
         periodTicks << 1;
      }
      retValues["periodTicks"] = periodTicks;
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

QVariant RtBoxXmlRpcServer::status(double aModelTimeStamp, double aLogPosition)
{
   return status(static_cast<int>(aModelTimeStamp), static_cast<int>(aLogPosition));
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
   readLineFile("/sys/bus/iio/devices/iio:device0/in_temp0_ps_temp_raw", temp);
   QVariantList temperatures;
   temperatures.append(QVariant((temp.toUInt() - 36058) * 7.771514892 / 1000));
   if (!mMeasureHwmonBoardTemp.isEmpty())
   {
      readLineFile(mMeasureHwmonBoardTemp, temp);
      temperatures.append(QVariant(temp.toUInt()/1000.0));
   }
   QVariantList fanSpeeds;
   QByteArray fanSpeed;
   readLineFile("/sys/class/hwmon/hwmon0/fan1_input", fanSpeed);
   fanSpeeds.append(fanSpeed.toInt());
   if (mHasCPUFan)
   {
      readLineFile("/sys/bus/i2c/devices/0-004b/hwmon/hwmon1/fan1_input", fanSpeed);
      fanSpeeds.append(fanSpeed.toInt());
   }
   QVariantList voltages;
   for (int i=1; i<4; i++)
   {
      QByteArray measurement;
      readLineFile(mMeasureHwmon1VoltInput.arg(i), measurement);
      voltages.append(QVariant(measurement.toInt()/1000.0));
   }
   QVariantList currents;
   for (int i=1; i<4; i++)
   {
      QByteArray measurement;
      readLineFile(mMeasureHwmon1CurrInput.arg(i), measurement);
      currents.append(QVariant(measurement.toInt()/1000.0));
   }
   if (!mMeasureHwmon2VoltInput.isEmpty())
   {
      QByteArray measurement;
      readLineFile(mMeasureHwmon2VoltInput, measurement);
      voltages.append(QVariant(measurement.toInt()/1000.0));
      for (int i=1; i<3; i++)
      {
         QByteArray measurement;
         readLineFile(mMeasureHwmon2CurrInput.arg(i), measurement);
         currents.append(QVariant(measurement.toInt()/1000.0));
      }
   }
   const QByteArray& msgBuffer = mSimulation.getMessageBuffer();
   QString log = msgBuffer.mid(logPosition);
   logPosition = msgBuffer.size();
   retValues["temperature"] = temperatures;
   retValues["fanSpeed"] = fanSpeeds;
   retValues["logPosition"] = logPosition;
   retValues["voltages"] = voltages;
   retValues["currents"] = currents;
   retValues["applicationLog"] = log;
   retValues["clearLog"] = clearLog;
   retValues["modelTimeStamp"] = mSimulation.getModelTimeStamp();
   return retValues;
}

QVariant RtBoxXmlRpcServer::start(double aStartOnFirstTrigger)
{
   return start(static_cast<int>(aStartOnFirstTrigger));
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
   retValues["rtboxType"] = mSimulation.isRtBox3() ? "PLECS RT Box 3" : "PLECS RT Box 2";
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
   QFile firmwareFile("/usr/lib/firmware/firmware");
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

QVariant RtBoxXmlRpcServer::loadElf(const QString &aData)
{
   return loadElf(QByteArray::fromBase64(aData.toLatin1()));
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

