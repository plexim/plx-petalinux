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

#ifndef _SERVERASYNC_H_
#define _SERVERASYNC_H_

#include <QtCore/QVector>
#include <QtCore/QObject>
#include "SimulationRPC.h"

class QTcpServer;
class QTcpSocket;
class QLocalServer;
class QLocalSocket;
class LocalServer;
class RtBoxXmlRpcServer;

class ServerAsync : public QObject
{
   Q_OBJECT;
   
public:
   enum MessageType
   {
      // packets sent by client
      
      REQUEST_MODELINFO = 0,
      REQUEST_SIGNALDATA,
      REQUEST_TUNE_PARAMS,
      
      // packets sent by server
      
      REPLY_MODELINFO,
      REPLY_SIGNALDATA,
      REPLY_TUNE_PARAMS,
      REPLY_ERROR
   };

   ServerAsync(quint16 port=0);
   ~ServerAsync();
   void receiveScopeData(const struct SimulationRPC::ArmResponse& aResp);
   void receiveTuneParametersResponse(int aErrorCode);
   void reportError(const QString& aError);
   void doLogging(const QString &aMsg);
   void closeConnection();  
   
   SimulationRPC *getSimulation() { return mSimulation; }

protected slots:
   void acceptConnection();
   void clientDisconnected();

   void readyRead();
   
 
protected:
   void armScope(struct SimulationRPC::ArmMessage& aArmMsg);
  
private:
   SimulationRPC* mSimulation;
   quint16 mTcpPort;
   QTcpServer *mTcpServer;
   QTcpSocket *mTcpSocket;
   LocalServer* mLocalServer;
   RtBoxXmlRpcServer* mXmlRpcServer;
   bool mConnected;
   qint32 mSamplePeriod; // FIXME: move to arm / response handshake data   
   double mSampleTime;
   int mNumScopeSignals;
   int mNumTuneableParameters;
   QByteArray mChecksum;
   QByteArray mModelName;
   SimulationRPC::VersionType mExeVersion;
};


#endif
