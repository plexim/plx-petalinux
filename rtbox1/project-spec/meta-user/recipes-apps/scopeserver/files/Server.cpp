/*
 * Copyright (c) 2015-2020 Plexim GmbH.
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

#include <QtCore/QDebug>
#include <QtCore/QDateTime>

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>

#include "Server.h"
#include "IOHelper.h"
#include "SimulationRPC.h"
#include "LocalServer.h"
#include "xml-rpc/RtBoxXmlRpcServer.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

ServerAsync::ServerAsync(quint16 port)
 : mSampleTime(0),
   mNumScopeSignals(0),
   mNumTuneableParameters(0)
{
   // Create TCP Server
   mTcpServer = new QTcpServer(this);
   connect(mTcpServer, &QTcpServer::newConnection, this, &ServerAsync::acceptConnection);
   
   if (!mTcpServer->listen(QHostAddress::Any, port))
   {
      doLogging(tr("Unable to start server: %1").arg(mTcpServer->errorString()));
      return;
   }
   mTcpPort = mTcpServer->serverPort();
   qDebug() << QString("The server is listening on port %1").arg(port);

   mConnected = false;
   mTcpSocket = NULL;
   mSimulation = new SimulationRPC(*this);
   mLocalServer = new LocalServer(*mSimulation);
   mXmlRpcServer = new RtBoxXmlRpcServer(*mSimulation, 9998, this);
}


ServerAsync::~ServerAsync()
{
   qDebug() << "** Destroying server ...\n";
   
   delete mLocalServer;
   delete mSimulation;
   delete mTcpServer;
   
   //    mSim->stop();
   //    mSim->wait();
}


void ServerAsync::doLogging(const QString &aMsg)
{
   QString logTime = QString("[%1]").arg(QDateTime::currentDateTime().time().toString("hh:mm:ss.zzz"));
   qDebug() << (logTime + aMsg).toLocal8Bit();
}


void ServerAsync::acceptConnection()
{
    QTcpSocket *socket = mTcpServer->nextPendingConnection();

    // Accept only one connection at a time
    if (mConnected)
    {
        socket->disconnectFromHost();
        socket->deleteLater();
        return;
    }
   
   mTcpSocket = socket;
   mTcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

   int sd = static_cast<int>(mTcpSocket->socketDescriptor());

   const int idleTimeSeconds = 2;
   const int intervalSeconds = 1;
   const int probeCount = 5;
   const int enableKeepAlive = 1;
   setsockopt(sd, IPPROTO_TCP, TCP_KEEPIDLE,  &idleTimeSeconds, sizeof(int));
   setsockopt(sd, IPPROTO_TCP, TCP_KEEPINTVL, &intervalSeconds, sizeof(int));
   setsockopt(sd, IPPROTO_TCP, TCP_KEEPCNT,   &probeCount,      sizeof(int));
   setsockopt(sd, SOL_SOCKET,  SO_KEEPALIVE,  &enableKeepAlive, sizeof(int));

   QHostAddress cliaddr = socket->peerAddress();
   connect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnected()));

   doLogging(QString("** Accepted connection from client at address %1").arg(cliaddr.toString()));
   
   mConnected = true;
   if (mSimulation->openConnection())
   {
      if (!mSimulation->querySimulation(mSampleTime, mNumScopeSignals, 
                                        mNumTuneableParameters, mExeVersion, 
                                        mChecksum, mModelName))
      {
         doLogging("Initial querySimulation failed.");
         closeConnection();
         return;
      }
      connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
      readyRead();
   }
   else
      closeConnection();
}

void ServerAsync::clientDisconnected()
{
    closeConnection();
    QObject *socket = sender();
    socket->deleteLater();

    doLogging(tr("** Connection to client closed"));
    mConnected = false;
    mSampleTime = 0;
    mNumScopeSignals = 0;
    mNumTuneableParameters = 0;
}


void ServerAsync::closeConnection()
{
   if (mTcpSocket)
   {  
      mTcpSocket->disconnect(SIGNAL(readyRead()));
      mTcpSocket->disconnectFromHost();
      mTcpSocket = NULL;
   }
}

void ServerAsync::reportError(const QString& aError)
{
   doLogging(aError);
   if (!mTcpSocket)
   {
      return;
   }
   QByteArray msg = aError.toUtf8();
   IOHelper ioHelper(*mTcpSocket);
       
   try
   {
      int msgType = REPLY_ERROR;
      ioHelper.writeAsync(&msgType);
      int msgLen = msg.size();
      ioHelper.writeAsync(&msgLen);
      ioHelper.writeAsync(msg.data(), msgLen);
   }
   catch (IOHelper::IOError &err)
   {
      doLogging(err.errMsg);
      closeConnection();
      return;
   }
}

void ServerAsync::readyRead()
{
   IOHelper ioHelper(*mTcpSocket,5000);

   bool itemRead;

   qint32 messageType;
   do
   {
      itemRead = ioHelper.readAsync(&messageType);
      if (!itemRead) { return; }
      // doLogging(QString("TCP request %1").arg(messageType));
      if (messageType == REQUEST_MODELINFO)
      {
         // doLogging(QString("---> Model info request"));
         
         int msgType = REPLY_MODELINFO;
         ioHelper.writeAsync(&msgType);
         ioHelper.writeAsync(&mSampleTime);
         int checksumLength = mChecksum.size();
         ioHelper.writeAsync(&checksumLength);
         ioHelper.writeAsync(mChecksum.data(), checksumLength);
         ioHelper.writeAsync(&mNumScopeSignals);
         ioHelper.writeAsync(&mNumTuneableParameters);
      }

      else if (messageType == REQUEST_TUNE_PARAMS)
      {
         int numParams;
         QByteArray data;
         try
         {
            ioHelper.read(&numParams);
            if (numParams > mNumTuneableParameters)
               throw IOHelper::IOError(QString("Invalid number of tuneable parameters %1.").arg(numParams));
            data.resize(3*sizeof(int) + numParams*sizeof(double));
            *(int*)(data.data() + 2*sizeof(int)) = numParams;
            ioHelper.read((double*)(data.data() + 3 * sizeof(int)), numParams);
         } 
         catch (IOHelper::IOError &err)
         {
            doLogging(QString("REQUEST_TUNE_PARAMS: ") + err.errMsg);
            closeConnection();
            return;
         }
         if (!mSimulation->tuneParameters(data))
         {
            quint32 msg = REPLY_TUNE_PARAMS;
            qint32 errCode = 1;
            ioHelper.writeAsync(&msg);
            ioHelper.writeAsync(&errCode);
         }
      }

      else if (messageType == REQUEST_SIGNALDATA)
      {
         QByteArray dataBuffer;
         dataBuffer.resize(sizeof(struct SimulationRPC::ArmMessage));
         struct SimulationRPC::ArmMessage* msg = (struct SimulationRPC::ArmMessage*)dataBuffer.data();
         qint32 requestedNumSamples = 0;
         try 
         {
            ioHelper.read(&msg->mTransactionId);
            ioHelper.read(&msg->mTriggerChannel);
            ioHelper.read(&msg->mTriggerEdge);
            ioHelper.read(&msg->mTriggerValue);
            ioHelper.read(&msg->mTriggerDelay);
            ioHelper.read(&requestedNumSamples);
            ioHelper.read(&mSamplePeriod);
            msg->mDecimation = (mSamplePeriod > 0) ? mSamplePeriod : 1;
            ioHelper.read(&msg->mNumActiveSignals);
            if (msg->mNumActiveSignals > mNumScopeSignals)
               throw IOHelper::IOError(QString("Invalid number of scope signals %1.").arg(msg->mNumActiveSignals));
            int newSize = offsetof(SimulationRPC::ArmMessage, mRequestedSignals) + msg->mNumActiveSignals*sizeof(quint16);
            dataBuffer.resize(newSize);
            msg = (struct SimulationRPC::ArmMessage*)dataBuffer.data(); // address may have changed
            QVector<qint32> requestedSignalIds;
            requestedSignalIds.resize(msg->mNumActiveSignals);
            ioHelper.read(requestedSignalIds.data(), msg->mNumActiveSignals);
            msg->mLength = msg->mNumActiveSignals * requestedNumSamples * sizeof(float);
            if (mTcpSocket->bytesToWrite() > 3*msg->mLength)
               continue; // avoid congestion
            quint16* requestedSignalsPtr = &msg->mRequestedSignals;
            for (int i=0; i<msg->mNumActiveSignals; i++)
            {
               requestedSignalsPtr[i] = requestedSignalIds[i];
            }
            msg->mMsgLength = dataBuffer.size();
         }
         catch (IOHelper::IOError &err)
         {
            doLogging(QString("REQUEST_SIGNALDATA: ") + err.errMsg);
            closeConnection();
            return;
         }
         // doLogging(QString("---> Arm request #%1: %2 samples, %3 signals").
         //           arg(msg->mTransactionId).arg(requestedNumSamples).arg(msg->mNumActiveSignals));
         
         armScope(*msg);
      }
      else 
      {
         reportError(QString("Unhandled message type %1, disconnect.").arg(messageType));
         closeConnection();
         return;
      }
 
   } while (itemRead);
}

void ServerAsync::receiveScopeData(const struct SimulationRPC::ArmResponse& aResp)
{
   if (!mTcpSocket)
      return; // TCP connection has been closed

   QVector<float> buffer(aResp.mLength / sizeof(float));
   qint32 errCode = 0;

   if (!mSimulation->getScopeBuffer(buffer, aResp.mBufferIndex, aResp.mOffset))
   {
       errCode = -1;
   }
      
   QVector<int32_t> sentSignalIds = QVector<int32_t>() << 0;
      
   IOHelper ioHelper(*mTcpSocket);
   
    
   try
   {
      int msgType = REPLY_SIGNALDATA;
      ioHelper.writeAsync(&msgType);
      ioHelper.writeAsync(&aResp.mTransactionId);
      ioHelper.writeAsync(&errCode);
      
      if (errCode != 0)
      {
         return;
      }
      qint32 samplesToSend = aResp.mLength / aResp.mNumActiveSignals / sizeof(float);
      ioHelper.writeAsync(&samplesToSend);
      
      float sampleTime = (float)aResp.mSampleTime * mSamplePeriod;
      ioHelper.writeAsync(&sampleTime);
      
//      float initialTime = aTriggerDelay * mSim->sampleTime();
//      ioHelper.writeAsync(&initialTime);
            
      QVector<qint32> signalIds(aResp.mNumActiveSignals);
      const quint16* activeSignals = &aResp.mActiveSignals;
      for (int i=0; i<aResp.mNumActiveSignals; ++i)
      {
         signalIds[i] = activeSignals[i];
      }
      
      ioHelper.writeAsync(&aResp.mNumActiveSignals);
      ioHelper.writeAsync(signalIds.data(), aResp.mNumActiveSignals);
      ioHelper.writeAsync(buffer.data(), aResp.mLength / sizeof(float));
      //doLogging(QString("Sent %1 samples to client").arg(samplesToSend));
      
   }
   catch (IOHelper::IOError &err)
   {
      doLogging(err.errMsg);
      closeConnection();
      return;
   }
}

void ServerAsync::receiveTuneParametersResponse(int aErrorCode)
{
   if (!mTcpSocket)
      return; // TCP connection has been closed
   IOHelper ioHelper(*mTcpSocket);
   quint32 msg = REPLY_TUNE_PARAMS;
   ioHelper.writeAsync(&msg);
   ioHelper.writeAsync(&aErrorCode);
}


void ServerAsync::armScope(struct SimulationRPC::ArmMessage& aArmMsg)
{
   aArmMsg.mMsg = 0x02; // ARM_SCOPE
   mSimulation->armScope(aArmMsg);
}

