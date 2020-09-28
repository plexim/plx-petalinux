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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdint>

#include <rpumsg.h>
#include "display_backend_messages.h"
#include "display_backend_parameters.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <QtCore/QSocketNotifier>

#include <QtGui/QKeyEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QLabel>

#include "hw_wrapper.h"

#include "Display.h"
#include "DisplayProxyStyle.h"
#include "Server.h"
#include "PageList.h"
#include "SystemLog.h"
#include "SimulationInfo.h"
#include "SimulationMessages.h"
#include "ImportantMessage.h"


void Display::exitBootMode()
{
    uint32_t msg_id = DISP_BACK_MSG_EXIT_BOOT_MODE;
    rpumsg_post_message(&msg_id, sizeof(msg_id));
    rpumsg_notify_remote();

    pokeBackend();
}


void Display::pokeBackend()
{
    uint32_t msg_id = DISP_BACK_MSG_POKE;
    rpumsg_post_message(&msg_id, sizeof(msg_id));
    rpumsg_notify_remote();
}


void Display::setDisplayStyle()
{
    qApp->setStyle(new DisplayProxyStyle);

    QFile styleFile(":/oled.qss");
    styleFile.open(QFile::ReadOnly);

    QString style(styleFile.readAll());
    setStyleSheet(style);
}


void Display::setOLEDContrast(int aContrast)
{
    static uint8_t buffer[4 + sizeof(struct disp_back_msg_contrast_param)];

    auto *msg_id = (uint32_t *)&buffer[0];
    auto *par = (struct disp_back_msg_contrast_param *)&buffer[4];

    *msg_id = DISP_BACK_MSG_OLED_CONTRAST;
    par->contrast = aContrast;
    rpumsg_post_message(&buffer[0], sizeof(buffer));
    rpumsg_notify_remote();
}


void Display::enableOLED(bool aEnabled)
{
    uint32_t msg_id;
    msg_id = aEnabled ? DISP_BACK_MSG_OLED_ON : DISP_BACK_MSG_OLED_OFF;
    rpumsg_post_message(&msg_id, 4);
    rpumsg_notify_remote();
}


void Display::maximizeWidget(QWidget *aWidget)
{
    aWidget->setFixedSize(this->size());
    aWidget->move(this->rect().center() - aWidget->rect().center());
}


void Display::activateWidget(QWidget *aWidget)
{
    mMainLayout->setCurrentWidget(aWidget);
    aWidget->setFocus();
    mPageListWidget->setCurrentPage(aWidget);
}


void Display::addPage(QWidget *aWidget, const QString &aDescription, bool aSelected)
{
    Q_ASSERT(mMainLayout != nullptr);
    Q_ASSERT(mPageListWidget != nullptr);

    mMainLayout->addWidget(aWidget);

    auto *item = new QListWidgetItem;
    item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(aWidget)));
    item->setText(aDescription);

    mPageListWidget->addItem(item);

    if (aSelected)
        mPageListWidget->setCurrentItem(item);
}


Display::Display(ServerAsync *aServer, QWidget *aParent)
: QWidget(aParent),
  mServer(aServer),
  mMainLayout(nullptr),
  mPageListWidget(nullptr)
{
    QCoreApplication::instance()->installEventFilter(this);

    enableOLED(true);
    setOLEDContrast(15);

    connect(qApp, &QCoreApplication::aboutToQuit, [this] () {
        this->enableOLED(false);
    });

    exitBootMode();
    setDisplayStyle();

    setFixedSize(256, 64);
    show();

    mMainLayout = new QStackedLayout;
    setLayout(mMainLayout);

    mImportantMessageWidget = new ImportantMessage(this);
    maximizeWidget(mImportantMessageWidget);
    mMainLayout->addWidget(mImportantMessageWidget);

    mPageListWidget = new PageList;
    maximizeWidget(mPageListWidget);
    mMainLayout->addWidget(mPageListWidget);
    connect(mPageListWidget, &PageList::activatePageRequest, this, &Display::activateWidget);

    auto *sysLogWidget = new SystemLog(mImportantMessageWidget);
    maximizeWidget(sysLogWidget);
    addPage(sysLogWidget, "System log", true);

    mSimulationInfoWidget = new SimulationInfo(mServer->getSimulation());
    maximizeWidget(mSimulationInfoWidget);
    addPage(mSimulationInfoWidget, "Simulation info");

    mSimulationMessagesWidget = new SimulationMessages;
    maximizeWidget(mSimulationMessagesWidget);
    addPage(mSimulationMessagesWidget, "Simulation messages");
    connect(mServer->getSimulation(), &SimulationRPC::simulationStartRequested, mSimulationMessagesWidget, &SimulationMessages::clear);
    connect(mServer->getSimulation(), &SimulationRPC::logMessage, this, &Display::handleSimulationMessage);

    /*
    for (auto i: {1,2,3,4,5,6})
    {
        QString label = QString("Test page %1").arg(i);
        auto *test = new QLabel(label);
        test->setAlignment(Qt::AlignCenter);
        maximizeWidget(test);
        addPage(test, label);
    }
    */

    activateWidget(sysLogWidget);
    //activateWidget(mSimulationInfoWidget);
    //activateWidget(mSimulationMessagesWidget);

    //mImportantMessageWidget->newMessage("Test message.");

    connect(mServer->getSimulation(), &SimulationRPC::simulationStarted, this, &Display::handleSimulationStarted);
}


bool Display::eventFilter(QObject *aObj, QEvent *aEvent)
{
    Q_UNUSED(aObj);

    bool filterEvent = false;

    if (aEvent->type() == QEvent::KeyPress)
    {
        if (read_display_off_flag() == 1)
            filterEvent = true;

        pokeBackend();
    }

    return filterEvent;
}


void Display::keyPressEvent(QKeyEvent *aEvent)
{
    if (aEvent->key() == Qt::Key_Backspace)
    {
        activateWidget(mPageListWidget);
    }
    else
    {
        QWidget::keyPressEvent(aEvent);
    }
}


void Display::handleSimulationStarted()
{
    activateWidget(mSimulationInfoWidget);
    pokeBackend();
}


int Display::read_display_off_flag()
{
    uint32_t display_off;
    rpumsg_shm_read(DISP_PARAM_DISPLAY_OFF_FLAG_OFS, &display_off, 4);

    return display_off;
}


void Display::handleSimulationMessage(int aFlags, QByteArray aMessage)
{
    QString msg = aMessage.simplified();

    mSimulationMessagesWidget->append(msg);

    if (aFlags & PLXUSERMSG_NEEDS_ATTENTION)
    {
        mImportantMessageWidget->newMessage(msg);
    }

    if (aFlags & PLXUSERMSG_CLOSE_MESSAGE_WINDOW)
    {
        mImportantMessageWidget->closeMessage();
    }
}


QWidget *Display::currentWidget() const
{
    return mMainLayout->currentWidget();
}
