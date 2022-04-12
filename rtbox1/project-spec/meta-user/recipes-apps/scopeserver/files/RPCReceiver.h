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

#ifndef RPCRECEIVER
#define RPCRECEIVER

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <list>

class SimulationRPC;
class QSocketNotifier;
class CanHandler;
class UdpTxHandler;
class UdpRxHandler;
class ToFileHandler;

class RPCReceiver : public QObject
{
   Q_OBJECT

public:
   RPCReceiver();
   virtual ~RPCReceiver();
   bool waitForMessage(char* aSendMsg, int aMsgSize, int aMessageId, QByteArray& aData, unsigned long aTimeout);
   void log(const QString& aMsg);
   void reportError(const QString& aMsg);
   inline void* getToFileBuffer() { return mToFileBuffer; }
   inline void setToFileBuffer(void* aBuffer) { mToFileBuffer = aBuffer; }

signals:
   void initComplete();
   void scopeArmResponse(QByteArray);
   void tuneParameterResponse(int);
   void finished();
   void error(QString err);
   void sigLog(QString);
   void sigError(QString);
   void sendRequest(const QByteArray);
   void simulationError();

public slots:
   void send(const QByteArray);
   void shutdown();
   void initializeToFileHandler(QString aFileName, QByteArray aModelName, int aWidth, int aNumSamples, int aBufferOffset, int aFileType);

protected slots:
   void receiveData();
   void process();

protected:
   bool readData(QByteArray& aData);
   bool processMessage(int aMessage, QByteArray& aMsgBuf);
   void readAll();
   void openConnection();

private:
   QFile mSimulationConnection;
   QSocketNotifier* mNotifier;
   QMutex mMutex;
   QWaitCondition mWaitCondition;
   int mCurrentMessageFilter;
   QByteArray mCurrentMessage;
   CanHandler* mCanHandlers[2];
   UdpTxHandler* mUdpTxHandler;
   std::list<UdpRxHandler*> mUdpRxHandlers;
   std::vector<ToFileHandler*> mToFileHandlers;
   void* mToFileBuffer;
};

#endif // RPCRECEIVER

