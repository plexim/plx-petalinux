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

#include "FanControl.h"
#include <QDebug>
#include <QtCore/QFileInfoList>
#include <QtCore/QDir>

FanControl::FanControl(QObject *parent) : QObject(parent),
   mTemperature(.0),
   mTempFile("/sys/bus/iio/devices/iio:device0/in_temp0_raw")
{
   QDir hwmonDir("/sys/bus/i2c/devices/0-0048/hwmon");
   QFileInfoList monEntries = hwmonDir.entryInfoList(QStringList() << "hwmon*");
   if (monEntries.isEmpty())
      qCritical("Cannot find device files for fan control.");
   else
   {
      mFanFile.setFileName(monEntries[0].canonicalFilePath()+"/fan1_target");
      if (mFanFile.open(QIODevice::ReadWrite) && mTempFile.open(QIODevice::ReadOnly))
         startTimer(5000);
      else
         qCritical("Cannot open device files for fan control.");
   }
}

FanControl::~FanControl()
{
   mFanFile.close();
   mTempFile.close();
}
void FanControl::timerEvent(QTimerEvent* aEvent)
{
   mTempFile.reset();
   QByteArray rawTemp = mTempFile.readLine().trimmed();
   mTemperature = rawTemp.toUInt() * 503.975 / 4096 - 273.15;
   unsigned int fanSpeed = mTemperature * 4000/30 - 4600;
   if (fanSpeed < 900)
      fanSpeed = 900;
   if (fanSpeed > 6000)
      fanSpeed = 6000;
   //qDebug() << fanSpeed << mTemperature << rawTemp;
   mFanFile.reset();
   mFanFile.write(QByteArray::number(fanSpeed));
   mFanFile.write("\n");
}
