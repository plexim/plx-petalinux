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

#ifndef RTBOXXMLRPCSERVER_H_
#define RTBOXXMLRPCSERVER_H_

class MaiaXmlRpcServer;
class QString;
class QByteArray;
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QDateTime>

class SimulationRPC;

class RtBoxXmlRpcServer : public QObject {
   Q_OBJECT

public:
   RtBoxXmlRpcServer(SimulationRPC& aSimulation, int aPort, QObject* aParent = NULL);
   virtual ~RtBoxXmlRpcServer();

protected slots:
   QVariant resetCounter();
   QVariant queryCounter();
   QVariant querySimulation();
   QVariant hostname();
   QVariant status(int aModelTimeStamp, int aLogPosition);
   QVariant start(int aStartOnFirstTrigger = 0);
   QVariant stop();
   QVariant serials();
   QVariant getDataCaptureTriggerCount(const QString& aDataCapturePath);
   QVariant getDataCaptureData(const QString& aDataCapturePath);
   QVariant setProgrammableValueData(const QString& aProgrammableValuePath, const QVariantList& aData);
   QVariant setProgrammableValueData(const QString& aProgrammableValuePath, double aData);
   QVariant setProgrammableValueData(const QString& aProgrammableValuePath, int aData);
   QVariant getProgrammableValueBlocks();
   QVariant getDataCaptureBlocks();
   QVariant loadElf(const QByteArray& aData);
   QVariant reboot(const QString& aSecret);

protected:
   void readLineFile(const QString& aFilename, QByteArray& aContent);

private:
   MaiaXmlRpcServer* mServer;
   SimulationRPC& mSimulation;
   int mCpuSerial;
   QString mBoardRevision;
   int mBoardSerial;
   QString mMacAddress;
   QString mFpgaVersion;
};


#endif
