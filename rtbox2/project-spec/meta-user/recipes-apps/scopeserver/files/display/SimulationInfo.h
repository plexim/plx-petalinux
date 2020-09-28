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

#ifndef SIMULATIONINFO_H
#define SIMULATIONINFO_H

#include <QWidget>

class SimulationRPC;
class QLabel;
class QStackedLayout;
class CpuLoad;
class QTimer;
class ElidedLabel;

class SimulationInfo : public QWidget
{
    Q_OBJECT

public:
    SimulationInfo(SimulationRPC *aSimulation, QWidget *aParent = nullptr);

protected slots:
    void handleSimulationStarted();
    void handleSimulationAboutToStop();
    void updateDynamicSimulationProperties();

protected:
    void updateStaticSimulationProperties();

    void showEvent(QShowEvent *aEvent) override;
    void hideEvent(QHideEvent *aEvent) override;

private:
    SimulationRPC *mSimulation;
    bool mSimulationRunning;
    double mSampleTime;

    QTimer *mDynamicSimulationPropertiesTimer;

    QStackedLayout *mStackedLayout;
    QLabel *mNoSimulationMessage;
    QWidget *mMainWidget;
    ElidedLabel *mModelText;
    QLabel *mStepText;
    CpuLoad *mCpuLoadWidgets[3];
};

#endif // SIMULATIONINFO_H
