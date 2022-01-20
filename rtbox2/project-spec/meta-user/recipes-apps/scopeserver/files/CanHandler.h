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

#ifndef _CANHANDLER_H_
#define _CANHANDLER_H_

#include <QtCore/QObject>
#include "SimulationRPC.h"

class RPCReceiver;
class QSocketNotifier;

class CanHandler : public QObject
{
   Q_OBJECT

public:
   CanHandler(int aCanModule, RPCReceiver* aParent);
   void canInit(const struct SimulationRPC::CanSetupMsg& aSetupMsg, int aMsgSize);
   void canTransmit(const struct SimulationRPC::CanTransmitMsg& aSetupMsg);
   void canStop();
   void canRequestId(const struct SimulationRPC::CanRequestIdMsg& aRequestMsg);

protected slots:
   void receiveCanData();

private:
   RPCReceiver* mParent;
   int mCanModule;
   const char* mDeviceName;
   int mCanFd;
   QSocketNotifier* mNotifier;
};

#endif

