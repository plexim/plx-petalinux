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

#include <string.h>
#include <QtCore/QTimer>
#include <QtCore/QDebug>
#include "PerformanceCounter.h"

#define BUFFER_LEN 16
#define UPDATE_PERIOD_MS (200)


PerformanceCounter::PerformanceCounter(QObject* aParent)
 : QObject(aParent),
   mCounter(nullptr),
   mBufferIdx(0)
{
   mUpdateTimer = new QTimer(this);
   mUpdateTimer->setInterval(UPDATE_PERIOD_MS);
   connect(mUpdateTimer, &QTimer::timeout, this, &PerformanceCounter::updateCounters);
   deinit();
}

void PerformanceCounter::init(uint32_t* aCounter, uint32_t* aPeriodTicks)
{
   mCounter = aCounter;
   for (int i=0; i<3; i++)
   {
      mPeriodTicks[i] = aPeriodTicks[i];
   }
   updateCounters();
   mUpdateTimer->start();
}

void PerformanceCounter::deinit()
{
   mUpdateTimer->stop();
   mCounter = nullptr;
   mBufferIdx = 0;
   memset(mCounterBuffer, 0, sizeof(mCounterBuffer));
   for (int i=0; i<3; i++)
   {
      mAbsMax[i] = 0;
      mPeriodTicks[i] = 0;
   }
}

PerformanceCounter::~PerformanceCounter()
{
}

uint32_t PerformanceCounter::getRunningMax(int aCore, int aNumInterval) const
{
   int startIdx = mBufferIdx - aNumInterval + 1;
   if (startIdx < 0)
      startIdx += BUFFER_LEN;
   uint32_t ret = 0;
   for (int i=0; i<aNumInterval; i++)
   {
      int idx = (startIdx + i) & (BUFFER_LEN-1);
      if (ret < mCounterBuffer[aCore][idx])
         ret = mCounterBuffer[aCore][idx];
   }
   return ret;
}

void PerformanceCounter::updateCounters()
{
   mBufferIdx = (mBufferIdx + 1) & (BUFFER_LEN-1);
   for (int i=0; i<3; i++)
   {
      mCounterBuffer[i][mBufferIdx] = mCounter[i];
      if (mAbsMax[i] < mCounter[i])
         mAbsMax[i] = mCounter[i];
      mCounter[i] = 0;
   }  
}

