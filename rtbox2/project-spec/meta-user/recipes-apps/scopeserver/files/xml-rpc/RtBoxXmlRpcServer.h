/*
 *  RtBoxXmlRpcServer.h
 *  server
 *
 *  Created by Oliver Schwartz on 27.7.2017.
 *  Copyright 2010 Plexim GmbH. All rights reserved.
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
