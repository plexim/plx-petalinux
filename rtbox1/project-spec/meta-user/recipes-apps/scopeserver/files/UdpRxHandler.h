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

#ifndef UDPRXHANDLER_H
#define UDPRXHANDLER_H

#include <QObject>
#include "SimulationRPC.h"
#include <QtCore/QSocketNotifier>

class RPCReceiver;
class QUdpSocket;

class UdpRxHandler : public QObject
{
   Q_OBJECT
public:
   explicit UdpRxHandler(uint16_t aPort, RPCReceiver *aParent);
   void stop();
   inline uint16_t getPort() const { return mPort; }

signals:

protected slots:
   void receiveData();

private:
   RPCReceiver* mParent;
   uint16_t mPort;
   QUdpSocket* mSocket;
};

#endif // UDPRXHANDLER_H
;
