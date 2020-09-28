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

#include "UdpTxHandler.h"

UdpTxHandler::UdpTxHandler(QObject *parent)
 : QObject(parent),
   mSendSocket(this)
{
   mSendSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1);
}

void UdpTxHandler::sendData(const char *aData, qint64 aSize, quint32 aAddress, quint16 aDestPort)
{
   mSendSocket.writeDatagram(aData, aSize, QHostAddress(aAddress), aDestPort);
}
