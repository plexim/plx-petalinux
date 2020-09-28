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


#include "LocalServerConnection.h"
#include "SimulationRPC.h"
#include "Server.h"
#include "IOHelper.h"
#include <QtNetwork/QLocalSocket>
#include <QtCore/QDebug>
#include "ReleaseInfo.h"
#include <cmath>


LocalServerConnection::LocalServerConnection(QLocalSocket& aConnection, 
                                             SimulationRPC& aSimulation, 
                                             QObject* aParent)
 : QObject(aParent), 
   mConnection(aConnection),
   mSimulation(aSimulation)
{
   connect(&mConnection, SIGNAL(disconnected()), this, SLOT(deleteLater()));
   connect(&mConnection, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

LocalServerConnection::~LocalServerConnection()
{
   mConnection.disconnect();
}

void LocalServerConnection::readyRead()
{
   unsigned int cmd;
   mConnection.read((char*)&cmd, sizeof(cmd));
   // qDebug() << QString("Local cmd: %1").arg(cmd).toLocal8Bit();
   if (cmd == 0xef56a55a)
   {
      mSimulation.shutdownSimulation();
      mConnection.write("\n");
   }
   else if (cmd == 0x00000005)
   {
      unsigned int startOnFirstTrigger;
      IOHelper helper(mConnection);
      helper.read(&startOnFirstTrigger);
      QString errMsg = mSimulation.startSimulation(startOnFirstTrigger);
      mConnection.write(errMsg.toLocal8Bit() + "\n");
   }
}

