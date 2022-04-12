/*
 * Copyright (c) 2021 Plexim GmbH.
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

#include "XcpHandler.h"
#include "RPCReceiver.h"
#include "SimulationRPC.h"
#include <QtCore/QSocketNotifier>
#include "xcpTl.h"
#include "xcpLite.h"

static RPCReceiver* mReceiverInstance;

XcpHandler::XcpHandler(uint16_t aPort, RPCReceiver *aParent)
 : QObject(aParent),
   mParent(aParent),
   mPort(aPort),
   mNotifier(nullptr)
{
   networkInit();
   gOptionSlavePort = aPort;
   udpTlInit(nullptr, aPort);
   mNotifier = new QSocketNotifier(gXcpTl.Sock.sock, QSocketNotifier::Read, this);
   connect(mNotifier, SIGNAL(activated(int)), this, SLOT(receiveData()));
   mReceiverInstance = aParent;
}


void XcpHandler::stop()
{
   udpTlShutdown();
   networkShutdown();
}

void XcpHandler::receiveData()
{
   mNotifier->setEnabled(false);
   udpTlHandleCommands();
   mNotifier->setEnabled(true);
}

extern "C" void XcpDisconnect()
{
   tXcpCto cmd;
   cmd.b[0] = CC_DISCONNECT;
   XcpCommand(cmd.dw);
}

/* XCP command processor */
extern "C" void XcpCommand( const vuint32* pCommand )
{
   QByteArray buf;
   buf.resize(sizeof(tXcpCto) + 2*sizeof(int));
   struct SimulationRPC::XcpCommandMsg* msg = (struct SimulationRPC::XcpCommandMsg*)buf.data();
   msg->mMsg = SimulationRPC::MSG_XCP_CMD;
   msg->mMsgLength = buf.size();
   memcpy(msg->mData, pCommand, sizeof(tXcpCto));
   mReceiverInstance->send(buf);
}

static uint32_t dummySlaveAddr;
uint32_t* gOptionSlaveAddr = &dummySlaveAddr;
uint16_t gOptionSlavePort = 0;

