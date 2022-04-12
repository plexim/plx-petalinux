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

#include "UdpRxHandler.h"
#include "RPCReceiver.h"
#include "SimulationRPC.h"
#include <QtNetwork/QUdpSocket>

UdpRxHandler::UdpRxHandler(uint16_t aPort, RPCReceiver *aParent)
 : QObject(aParent),
   mParent(aParent),
   mPort(aPort),
   mSocket(nullptr)
{
   mSocket = new QUdpSocket(this);
   mSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
   if (mSocket->bind(aPort))
   {
      connect(mSocket, &QAbstractSocket::readyRead, this, &UdpRxHandler::receiveData);
      mParent->log(QString("Listening on port %1").arg(aPort));
   }
   else
   {
      mParent->log(QString("Cannot bind to port %1").arg(aPort));
   }
}

void UdpRxHandler::stop()
{
   if (mSocket)
   {
      mSocket->close();
      delete mSocket;
      mSocket = nullptr;
   }

}

void UdpRxHandler::receiveData()
{
   struct UdpReceiveMsg
   {
      int mMsg;
      int mMsgLength;
      struct SimulationRPC::UdpReceiveMsg transmitMsg;
   };

   QByteArray buf;
   while (mSocket->hasPendingDatagrams())
   {
      int dataSize = mSocket->pendingDatagramSize();
      buf.resize(sizeof(struct UdpReceiveMsg) + dataSize - sizeof(uint32_t));
      struct UdpReceiveMsg* msg = (struct UdpReceiveMsg*)buf.data();
      msg->mMsg = SimulationRPC::UDP_RECEIVE;
      msg->mMsgLength = buf.length();
      msg->transmitMsg.mDataLength = dataSize;
      msg->transmitMsg.mPort = mPort;

      int readBytes = mSocket->readDatagram(buf.data() + offsetof(struct UdpReceiveMsg, transmitMsg) +
                           offsetof(struct  SimulationRPC::UdpReceiveMsg, mData),
                           dataSize);
      if (readBytes != dataSize)
      {
         return;
      }
      mParent->send(buf);
   }
}
