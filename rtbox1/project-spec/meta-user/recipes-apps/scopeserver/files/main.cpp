/*
 * Copyright (c) 2015-2020 Plexim GmbH.
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

#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#include "Server.h"
#include <cstdio>


#define SERVER_PORT 9999


int main(int argc, char *argv[])
{
   QCoreApplication app(argc, argv);
   if (app.arguments().contains("--detach"))
   {
      const QStringList args = QStringList() << "-use-logfile";
      QProcess::startDetached(app.arguments()[0], args);
      return 0;
   }
   else 
   {
      if (app.arguments().contains("-use-logfile"))
      {
         freopen("/var/log/scopeserver.log", "w", stderr);
      }
      // Create TCP Server
      ServerAsync server(SERVER_PORT);
    
      return app.exec();
   }
}
