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
#ifndef TEMPERATURECONTROL_H
#define TEMPERATURECONTROL_H

#include <QObject>
#include <QFile>

class SimulationRPC;

class TemperatureControl : public QObject
{
   Q_OBJECT
public:
   explicit TemperatureControl(SimulationRPC* aSimulation, bool aControlFan);
   virtual ~TemperatureControl();

   float getTemperature() const { return mTemperature; }

signals:
   void displayMessageRequest(int aFlags, QByteArray aMessage);

public slots:

protected:
   virtual void timerEvent(QTimerEvent* aEvent) override;
   void displayMessage(const QString& aMessage);

private:
   SimulationRPC* mSimulation;
   const bool mControlFan;
   float mTemperature;
   QFile mTempFile;
   QFile mCaseFanFile;
   QFile mCPUFanFile;
   bool mHasCPUFan;
};

#endif // TEMPERATURECONTROL_H
