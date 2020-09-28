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
#ifndef FANCONTROL_H
#define FANCONTROL_H

#include <QObject>
#include <QFile>

class FanControl : public QObject
{
   Q_OBJECT
public:
   explicit FanControl(QObject *parent = nullptr);
   virtual ~FanControl();

   float getTemperature() const { return mTemperature; }

signals:

public slots:

protected:
   virtual void timerEvent(QTimerEvent* aEvent) override;

private:
   float mTemperature;
   QFile mTempFile;
   QFile mFanFile;
};

#endif // FANCONTROL_H
