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

#ifndef CPULOAD_H
#define CPULOAD_H

#include <QtWidgets/QWidget>

class CpuIdLabel;
class CpuLoadLabel;
class CpuLoadBar;
class ActivateableLabel;
class QGridLayout;

class CpuLoad
{
public:
    CpuLoad(int aCpu, QGridLayout* aLayout);

    void setLoad(double aLoad);
    void setSampleTime(double aSampleTime);

private:
    CpuIdLabel *mIdLabel;
    CpuLoadLabel *mLoadLabel;
    CpuLoadBar *mLoadBar;
    ActivateableLabel *mSampleTimeLabel;
};

#endif // CPULOAD_H
