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

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>

#include "SimulationRPC.h"
#include "SimulationInfo.h"
#include "CpuLoad.h"
#include "ElidedLabel.h"
#include "PerformanceCounter.h"

#define UPDATE_PERIOD_MS (200)


SimulationInfo::SimulationInfo(SimulationRPC *aSimulation, QWidget *aParent)
: QWidget(aParent),
  mSimulation(aSimulation),
  mSimulationRunning(false),
  mSampleTime(0)
{
    mStackedLayout = new QStackedLayout;
    setLayout(mStackedLayout);

    mNoSimulationMessage = new QLabel("No simulation running");
    mNoSimulationMessage->setAlignment(Qt::AlignCenter);
    mStackedLayout->addWidget(mNoSimulationMessage);

    mMainWidget = new QWidget;
    mStackedLayout->addWidget(mMainWidget);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(2, 4, 2, 4);
    mainLayout->setSpacing(0);

    mMainWidget->setLayout(mainLayout);

    mModelText = new ElidedLabel;
    //mModelText->setAlignment(Qt::AlignLeft);
    //mModelText->setStyleSheet("border: 1px solid white");

    mStepText = new QLabel;
    mStepText->setAlignment(Qt::AlignRight);
    //mStepText->setStyleSheet("border: 1px solid white");

    auto *topRowLayout = new QHBoxLayout;
    topRowLayout->addWidget(mModelText);
    topRowLayout->addWidget(mStepText);
    //topRowLayout->setContentsMargins(2, 4, 2, 4);
    topRowLayout->setSpacing(12);
    mainLayout->addLayout(topRowLayout);

    mainLayout->addSpacing(4);

    for (auto i: {0, 1, 2})
    {
        mCpuLoadWidgets[i] = new CpuLoad(QString("CPU%1").arg(i+1));
        mainLayout->addWidget(mCpuLoadWidgets[i]);
    }

    mStackedLayout->setCurrentWidget(mNoSimulationMessage);
    //mStackedLayout->setCurrentWidget(mMainWidget);

    mDynamicSimulationPropertiesTimer = new QTimer(this);
    mDynamicSimulationPropertiesTimer->setInterval(UPDATE_PERIOD_MS);
    connect(mDynamicSimulationPropertiesTimer, &QTimer::timeout, this, &SimulationInfo::updateDynamicSimulationProperties);

    connect(mSimulation, &SimulationRPC::simulationStarted, this, &SimulationInfo::handleSimulationStarted);
    connect(mSimulation, &SimulationRPC::simulationAboutToStop, this, &SimulationInfo::handleSimulationAboutToStop);
}


void SimulationInfo::handleSimulationStarted()
{
    mSimulationRunning = true;
    updateStaticSimulationProperties();
    mStackedLayout->setCurrentWidget(mMainWidget);

    updateDynamicSimulationProperties();
    mDynamicSimulationPropertiesTimer->start();
}


void SimulationInfo::handleSimulationAboutToStop()
{
    mSimulationRunning = false;
    mStackedLayout->setCurrentWidget(mNoSimulationMessage);

    mDynamicSimulationPropertiesTimer->stop();
}


void SimulationInfo::updateStaticSimulationProperties()
{
    int numScopeSignals;
    int numTuneableParameters;
    struct SimulationRPC::VersionType libraryVersion;
    QByteArray checksum;
    QByteArray modelName;
    int analogOutVoltageRange;
    int analogInVoltageRange;
    int digitalOutVoltage;

    bool success = mSimulation->querySimulation(mSampleTime, numScopeSignals,numTuneableParameters, 
                                                libraryVersion,checksum, modelName, analogOutVoltageRange,
                                                analogInVoltageRange, digitalOutVoltage);
    if (success)
    {
        mModelText->setText(modelName);
        mStepText->setText(QString("%1 µs").arg(mSampleTime * 1e6, 0, 'f', 1));
    }
    else
    {
        mModelText->setText("-");
        mStepText->setText(QString("- µs"));
    }
}


void SimulationInfo::updateDynamicSimulationProperties()
{
    const auto& counter = mSimulation->getPerformanceCounter();
    if (counter && counter->isValid())
    {
        for (auto i: {0, 1, 2})
        {
            auto cycles = counter->getRunningMax(i);
            auto period = counter->getPeriodTicks(i);

            if (period > 0)
                mCpuLoadWidgets[i]->setLoad(static_cast<double>(cycles)/period);
            else
                mCpuLoadWidgets[i]->setLoad(0);
        }
    }
}


void SimulationInfo::showEvent(QShowEvent *aEvent)
{
    Q_UNUSED(aEvent);

    if (mSimulationRunning)
    {
        updateDynamicSimulationProperties();
        mDynamicSimulationPropertiesTimer->start();
    }
}


void SimulationInfo::hideEvent(QHideEvent *aEvent)
{
    Q_UNUSED(aEvent);

    if (mSimulationRunning)
    {
        mDynamicSimulationPropertiesTimer->stop();
    }
}
