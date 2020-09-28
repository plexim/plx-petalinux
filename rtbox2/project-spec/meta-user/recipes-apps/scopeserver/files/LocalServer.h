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

#ifndef LOCAL_SERVER_H_
#define LOCAL_SERVER_H_

#include <QtCore/QObject>

class QLocalServer;
class SimulationRPC;

class LocalServer : public QObject
{
   Q_OBJECT;

public:
   LocalServer(SimulationRPC& aSimulation);
   ~LocalServer();

protected slots:
   void acceptConnection();

   
private:
   QLocalServer* mLocalServer;
   SimulationRPC& mSimulation;   
};   
#endif

