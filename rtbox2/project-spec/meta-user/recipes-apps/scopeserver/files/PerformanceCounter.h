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

#ifndef _PERFORMANCCOUNTER_H_
#define _PERFORMANCCOUNTER_H_

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <stdint.h>

class QTimer;

class PerformanceCounter : public QObject
{
   Q_OBJECT

public:
   PerformanceCounter(QObject* aParent);
   virtual ~PerformanceCounter() override;
   void init(uint32_t* aCounter, uint32_t* aPeriodTicks);
   void deinit();
   uint32_t getRunningMax(int aCore, int aNumInterval = 1) const;
   inline uint32_t getAbsMax(int aCore) const { return mAbsMax[aCore]; }
   inline void resetMax() { for (auto i=0; i<3; i++) mAbsMax[i] = 0; }
   inline bool isValid() const { return mCounter; }
   inline uint32_t getPeriodTicks(int aCore) const { return mPeriodTicks[aCore]; }

protected slots:
   void updateCounters();

private:
   uint32_t* mCounter;
   uint32_t mCounterBuffer[3][16];
   int mBufferIdx;
   uint32_t mAbsMax[3];
   uint32_t mPeriodTicks[3];
   QPointer<QTimer> mUpdateTimer;
};

#endif

