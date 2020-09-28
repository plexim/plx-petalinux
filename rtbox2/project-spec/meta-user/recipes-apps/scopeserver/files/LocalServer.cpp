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

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QFile>

#include <QtNetwork/QLocalServer>

#include "LocalServer.h"
#include "LocalServerConnection.h"

static const QString socketFileName("/tmp/hilserver");

LocalServer::LocalServer(SimulationRPC& aSimulation)
 : mSimulation(aSimulation)
{
   QFile socketFile(socketFileName);
   if (socketFile.exists())
      socketFile.remove();
   // Create Server
   mLocalServer = new QLocalServer(this);
   connect(mLocalServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
   
   if (!mLocalServer->listen(socketFileName))
   {
      qWarning() << tr("Unable to start server: %1").arg(mLocalServer->errorString());
      return;
   }
}

LocalServer::~LocalServer()
{
}

void LocalServer::acceptConnection()
{
   QLocalSocket *socket = mLocalServer->nextPendingConnection();
   new LocalServerConnection(*socket, mSimulation, this);
}

