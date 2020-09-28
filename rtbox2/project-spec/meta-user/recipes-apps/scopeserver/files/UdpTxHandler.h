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

#ifndef UDPTXHANDLER_H
#define UDPTXHANDLER_H

#include <QtCore/QObject>
#include <QtNetwork/QUdpSocket>

class UdpTxHandler : public QObject
{
   Q_OBJECT
public:
   explicit UdpTxHandler(QObject *parent = 0);
   void sendData(const char *aData, qint64 aSize, quint32 aAddress, quint16 aDestPort);
signals:

public slots:

private:
   QUdpSocket mSendSocket;
};

#endif // UDPTXHANDLER_H
