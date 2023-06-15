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

#include "TemperatureControl.h"
#include "SimulationRPC.h"
#include "hw_wrapper.h"

#include <QDebug>

/*
 * Temperature above which a running simulation
 * should be stopped.
 */
static constexpr float SIMULATION_STOP_TEMPERATURE_THRESHOLD = 100;


TemperatureControl::TemperatureControl(SimulationRPC* aSimulation, bool aControlFan)
 : QObject(aSimulation),
   mSimulation(aSimulation),
   mControlFan(aControlFan),
   mTemperature(.0),
   mTempFile("/sys/bus/iio/devices/iio:device0/in_temp0_ps_temp_raw"),
   mCaseFanFile("/sys/bus/i2c/devices/0-0048/hwmon/hwmon0/fan1_target"),
   mCPUFanFile("/sys/bus/i2c/devices/0-004b/hwmon/hwmon1/fan1_target")
{
   mHasCPUFan = mCPUFanFile.exists();
   if (mCaseFanFile.open(QIODevice::ReadWrite) && mTempFile.open(QIODevice::ReadOnly))
      startTimer(5000);
   else
      qCritical("Cannot open device files for fan control.");
   if (mHasCPUFan)
      mCPUFanFile.open(QIODevice::ReadWrite);
}

TemperatureControl::~TemperatureControl()
{
   mCaseFanFile.close();
   mTempFile.close();
   if (mHasCPUFan)
      mCPUFanFile.close();
}

void TemperatureControl::timerEvent(QTimerEvent* aEvent)
{
   Q_UNUSED(aEvent);

   mTempFile.reset();
   QByteArray rawTemp = mTempFile.readLine().trimmed();
   mTemperature = (rawTemp.toUInt() - 36058) * 7.771514892 / 1000;

   if (mControlFan)
   {
      unsigned int fanSpeed = (mTemperature - 60) * 4000/15;
      if (fanSpeed < 1200)
         fanSpeed = 1200;
      if (fanSpeed > 6000)
         fanSpeed = 6000;
      //qDebug() << fanSpeed << mTemperature << rawTemp;
      mCaseFanFile.reset();
      mCaseFanFile.write(QByteArray::number(fanSpeed));
      mCaseFanFile.write("\n");
      if (mHasCPUFan)
      {
         mCPUFanFile.reset();
         mCPUFanFile.write(QByteArray::number(fanSpeed*2800/5000));
         mCPUFanFile.write("\n");
      }
   }

   if (mTemperature > SIMULATION_STOP_TEMPERATURE_THRESHOLD)
   {
      if (mSimulation->getStatus() == SimulationRPC::SimulationStatus::RUNNING)
      {
         mSimulation->shutdownSimulation();
         displayMessage(
            QString("CPU temperature exceeded %1°C, simulation has been stopped")
            .arg(SIMULATION_STOP_TEMPERATURE_THRESHOLD)
         );
      }
   }
}


void TemperatureControl::displayMessage(const QString &aMessage)
{
   emit displayMessageRequest(PLXUSERMSG_NEEDS_ATTENTION, aMessage.toUtf8());
}
