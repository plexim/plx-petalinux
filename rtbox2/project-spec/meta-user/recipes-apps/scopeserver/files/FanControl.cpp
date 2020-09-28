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

FanControl::FanControl(QObject *parent) : QObject(parent),
   mTemperature(.0),
   mTempFile("/sys/bus/iio/devices/iio:device0/in_temp0_ps_temp_raw"),
   mFanFile("/sys/class/hwmon/hwmon0/fan1_target")
{
   if (mFanFile.open(QIODevice::ReadWrite) && mTempFile.open(QIODevice::ReadOnly))
      startTimer(5000);
   else
      qCritical("Cannot open device files for fan control.");
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
   mTemperature = (rawTemp.toUInt() - 36058) * 7.771514892 / 1000;
   unsigned int fanSpeed = mTemperature * 4000/15 - 18000;
   if (fanSpeed < 900)
      fanSpeed = 900;
   if (fanSpeed > 6000)
      fanSpeed = 6000;
   //qDebug() << fanSpeed << mTemperature << rawTemp;
   mFanFile.reset();
   mFanFile.write(QByteArray::number(fanSpeed));
   mFanFile.write("\n");
}
