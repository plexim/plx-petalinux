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

#include "CanHandler.h"
#include "RPCReceiver.h"

#include <libsocketcan.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <QtCore/QDebug>
#include <QtCore/QSocketNotifier>
#include <QtCore/QFile>
#include <QtCore/QDir>

static const char* canDevNames[2] = { "can0", "can1" };

CanHandler::CanHandler(int aCanModule, RPCReceiver* aParent)
 : QObject(aParent),
   mParent(aParent),
   mCanModule(aCanModule),
   mDeviceName(canDevNames[aCanModule]),
   mCanFd(0),
   mNotifier(nullptr)
{
}

void CanHandler::canInit(const struct SimulationRPC::CanSetupMsg& aSetupMsg, int aMsgSize)
{
   if (mCanFd)
      return;

   // setup CAN termination for internal CAN interface
   if (aSetupMsg.mTermination >= 0 && aSetupMsg.mTermination <= 1)
   {
      if (QFile::exists("/sys/bus/i2c/devices/0-0021/gpio"))
      {
         QDir ioDir("/sys/bus/i2c/devices/0-0021/gpio");
         QStringList chips = ioDir.entryList(QStringList() << "gpiochip*");
         if (!chips.isEmpty())
         {
            int baseAddr = chips[0].mid(8).toInt();
            QString dirName = QString("/sys/class/gpio/gpio%1").arg(baseAddr+12+aSetupMsg.mModuleId);
            QFile valueFile(dirName + "/value");
            valueFile.open(QIODevice::WriteOnly);
            valueFile.write(aSetupMsg.mTermination ? "1\n" : "0\n");
            valueFile.close();
         }
      }
   }
   can_set_bitrate(mDeviceName, aSetupMsg.mBaudRate);
   if (aMsgSize == sizeof(struct SimulationRPC::CanSetupMsg)) // check for compatibility within 2.1.x releases
   {
      can_set_restart_ms(mDeviceName, aSetupMsg.mRecoveryTimeout);
   }
   if (can_do_start(mDeviceName) < 0)
   {
      mParent->reportError(QString("Error while starting CAN interface: ") + strerror(errno));
      return;
   }

   struct sockaddr_can addr;
   struct ifreq ifr;

   mCanFd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
   if (mCanFd < 0)
   {
      mParent->reportError(QString("Error while opening CAN socket: ") + strerror(errno));
      mCanFd = 0;
      return;
   }
   strcpy(ifr.ifr_name, mDeviceName);
   ioctl(mCanFd, SIOCGIFINDEX, &ifr);

   addr.can_family = AF_CAN;
   addr.can_ifindex = ifr.ifr_ifindex;

   if (bind(mCanFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
   {
      mParent->log(QString("Error while binding CAN socket: ") + strerror(errno));
   }
   mNotifier = new QSocketNotifier(mCanFd, QSocketNotifier::Read, this);
   connect(mNotifier, &QSocketNotifier::activated, this, &CanHandler::receiveCanData);
}

void CanHandler::canTransmit(const struct SimulationRPC::CanTransmitMsg& aMsg)
{
   if (aMsg.mDataLength)
      write(mCanFd, &aMsg.mCanId, sizeof(struct can_frame));
}

void CanHandler::canStop()
{
   if (mNotifier)
   {
      delete mNotifier;
      mNotifier = nullptr;
   }
   if (mCanFd)
      close(mCanFd);
   mCanFd = 0;
   can_do_stop(mDeviceName);
}

void CanHandler::canRequestId(const struct SimulationRPC::CanRequestIdMsg& aRequestMsg)
{
   size_t bufLength = sizeof(struct can_filter) * aRequestMsg.mNumCanIds;
   void* buf = malloc(bufLength);
   struct can_filter* rfilter = (struct can_filter*) buf;

   QString filteredIds;
   for (uint32_t i=0; i<aRequestMsg.mNumCanIds; i++)
   {
      if (i > 0)
         filteredIds += ", ";
      filteredIds += "0x" + QString::number(*(&aRequestMsg.mCanId + i) & CAN_EFF_MASK, 16);
      rfilter[i].can_id   = *(&aRequestMsg.mCanId + i);
      rfilter[i].can_mask = CAN_RTR_FLAG | CAN_EFF_FLAG;
      if (rfilter[i].can_id & CAN_EFF_FLAG)
      {
         rfilter[i].can_mask |= CAN_EFF_MASK;
         filteredIds += " (EFF)";
      }
      else
      {
         rfilter[i].can_mask |= CAN_SFF_MASK;
      }
   }
   qDebug() << mDeviceName << ": Filtering for IDs " << filteredIds.toLocal8Bit().data();
   setsockopt(mCanFd, SOL_CAN_RAW, CAN_RAW_FILTER, rfilter, bufLength);
   free(buf);
}

void CanHandler::receiveCanData()
{
   struct CanReceiveMsg
   {
      int mMsg;
      int mMsgLength;
      struct SimulationRPC::CanTransmitMsg transmitMsg;
   };

   QByteArray buf;
   buf.resize(sizeof (struct CanReceiveMsg));
   struct CanReceiveMsg* msg = (struct CanReceiveMsg*)buf.data();
   msg->mMsg = SimulationRPC::CAN_RECEIVE;
   msg->mMsgLength = buf.length();
   msg->transmitMsg.mModuleId = mCanModule;


   int readBytes = read(mCanFd,
                        buf.data() + offsetof(struct CanReceiveMsg, transmitMsg) +
                        offsetof(struct  SimulationRPC::CanTransmitMsg, mCanId),
                        sizeof(struct can_frame));
   if (readBytes != sizeof(struct can_frame))
   {
      return;
   }
   mParent->send(buf);
}
