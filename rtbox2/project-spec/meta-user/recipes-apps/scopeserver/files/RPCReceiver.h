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

#ifndef RPCRECEIVER
#define RPCRECEIVER

#include <QtCore/QObject>
#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <list>
#include <vector>

class SimulationRPC;
class QSocketNotifier;
class CanHandler;
class UdpTxHandler;
class UdpRxHandler;
class ToFileHandler;
struct MsgQueue;

class RPCReceiver : public QObject
{
   Q_OBJECT

public:
   RPCReceiver();
   virtual ~RPCReceiver();
   bool waitForMessage(char* aSendMsg, int aMsgSize, int aMessageId, QByteArray& aData, unsigned long aTimeout);
   void log(const QString& aMsg);
   void reportError(const QString& aMsg);
   bool mapBuffers(int aScopeBufferSize, int aToFileBufferSize, int aRxTxBufferSize);
   inline void* getScopeBuffer() { return mScopeBuffer; }
   inline volatile void* getRxBuffer() { return mRxBuffer; }
   inline void* getTxBuffer() { return mTxBuffer; }
   inline void* getToFileBuffer() { return mToFileBuffer; }
   inline uint32_t* getCpuPerformanceCounters()  { return mCpuPerformanceCounters; }
   void reportErrorMessage(const QString& aMsg);

signals:
   void initComplete();
   void scopeArmResponse(QByteArray);
   void tuneParameterResponse(int);
   void finished();
   void error(QString err);
   void sigLog(QString);
   void sigError(QString);
   void sendRequest(const QByteArray);
   void logMessage(int, QByteArray);
   void simulationError();

public slots:
   void send(const QByteArray);
   void shutdown();
   void initializeToFileHandler(QString aFileName, QByteArray aModelName, int aWidth, int aNumSamples, int aBufferOffset, int aFileType, int aWriteDevice);

protected slots:
   void receiveData();
   void process();

protected:
   bool readMsgData(QByteArray& aData);
   int readTail(QByteArray& aData);
   bool processMessage(int aMessage);
   void readAll();
   bool reconnect();
   void close();
   bool sendMsg(const char* aData, int aLength);
   static QString findUIODevice();

private:
   struct MsgQueue* mSendQueue;
   struct MsgQueue* mReceiveQueue;
   volatile void* mRxBuffer;
   void* mTxBuffer;
   void* mScopeBuffer;
   void* mToFileBuffer;
   QFile mSimulationConnection;
   QSocketNotifier* mNotifier;
   QMutex mMutex;
   QWaitCondition mWaitCondition;
   int mCurrentMessageFilter;
   QByteArray mCurrentMessage;
   CanHandler* mCanHandlers[2];
   UdpTxHandler* mUdpTxHandler;
   std::list<UdpRxHandler*> mUdpRxHandlers;
   uint32_t* mCpuPerformanceCounters;
   std::vector<ToFileHandler*> mToFileHandlers;
};

#endif // RPCRECEIVER

