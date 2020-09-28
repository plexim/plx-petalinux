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

#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QDebug>

#include <QtWidgets/QApplication>

#include <cstdio>
#include <cstdlib>
#include <memory>

#include <metal/sys.h>
#include <rpumsg.h>

#include "UnixSignalHandler.h"
#include "Server.h"
#include "display/Display.h"
#include "display_backend_parameters.h"


#define SERVER_PORT 9999

/* communication channel with the R5 display backend */
#define RPUMSG_SHM_DEVNAME (DISP_PARAM_SHM_LINUX_DEVNAME)
#define RPUMSG_SHM_BUFOFS  (DISP_PARAM_SHM_MSGBUF_OFS)
#define RPUMSG_SHM_BUFSZ   (DISP_PARAM_SHM_MSGBUF_SIZE)
#define RPUMSG_IPI_DEVNAME (DISP_PARAM_IPI_LINUX_DEVNAME)
#define RPUMSG_IPI_RMTCHN  (DISP_PARAM_IPI_R5_CHN)


static int initialize_metal_rpumsg()
{
    struct metal_init_params init_param = METAL_INIT_DEFAULTS;
    if (metal_init(&init_param) < 0)
    {
        qCritical() << "ERROR: Cannot initialize libmetal.";
        return -1;
    }

    atexit(metal_finish);

    struct rpumsg_params par = {
        .shm_dev_name = RPUMSG_SHM_DEVNAME,
        .shm_msgbuf_offset = RPUMSG_SHM_BUFOFS,
        .shm_msgbuf_size = RPUMSG_SHM_BUFSZ,
        .ipi_dev_name = RPUMSG_IPI_DEVNAME,
        .ipi_chn_remote = RPUMSG_IPI_RMTCHN
    };

    if (rpumsg_initialize(&par) < 0)
    {
        qCritical() << "ERROR: Cannot initialize rpumsg.";
        return -1;
    }

    atexit((void(*)())rpumsg_terminate);

    return 0;
}


static void set_platform_plugin()
{
    /*
    qputenv("QT_QPA_PLATFORM", "rtbox_oled:debug");
    qputenv("QT_LOGGING_RULES", "qt.qpa.rtbox=true");
    */
    qputenv("QT_QPA_PLATFORM", "rtbox_oled");

    qputenv("QT_QPA_EVDEV_KEYBOARD_PARAMETERS", "/dev/input/by-id/display-buttons:grab=1");
}


static void disable_display()
{
    Display::exitBootMode();
    Display::enableOLED(false);
}


int main(int argc, char *argv[])
{
   set_platform_plugin();

   QApplication app(argc, argv);

   UnixSignalHandler signalHandler;

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

      if (initialize_metal_rpumsg() < 0)
          return -1;

      // Create TCP Server
      ServerAsync server(SERVER_PORT);

      bool displayDisabled =
         app.arguments().contains("--no-display") ||
         ! qgetenv("SCOPESERVER_DISABLE_DISPLAY").isNull();

      std::unique_ptr<Display> display;
      if (displayDisabled)
      {
         disable_display();
         qDebug() << "Display disabled.";
      }
      else
      {
         display.reset(new Display(&server));
      }

      return app.exec();
   }
}
