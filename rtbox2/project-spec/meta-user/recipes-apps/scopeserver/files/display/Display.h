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

#ifndef DISPLAY_H
#define DISPLAY_H

#include <QtWidgets/QWidget>

class ServerAsync;
class QStackedLayout;
class PageList;
class SimulationInfo;
class SimulationMessages;
class ImportantMessage;

class Display: public QWidget
{
    Q_OBJECT

public:
    Display(ServerAsync *aServer, QWidget *aParent = nullptr);

    QWidget *currentWidget() const;

    static void pokeBackend();
    static void exitBootMode();
    static void setOLEDContrast(int aContrast);
    static void enableOLED(bool aEnabled);

protected:

    void setDisplayStyle();

    void maximizeWidget(QWidget *aWidget);

    void addPage(QWidget *aWidget, const QString &aDescription, bool aSelected = false);

    bool eventFilter(QObject *aObj, QEvent *aEvent) override;
    void keyPressEvent(QKeyEvent *aEvent) override;

    int read_display_off_flag();

public slots:
    void activateWidget(QWidget *aWidget);

protected slots:
    void handleSimulationStarted();
    void handleSimulationMessage(int aFlags, QByteArray aMessage);

private:
    ServerAsync *mServer;

    QStackedLayout *mMainLayout;
    PageList *mPageListWidget;

    SimulationInfo *mSimulationInfoWidget;
    SimulationMessages *mSimulationMessagesWidget;
    ImportantMessage *mImportantMessageWidget;
};

#endif // DISPLAY_H
