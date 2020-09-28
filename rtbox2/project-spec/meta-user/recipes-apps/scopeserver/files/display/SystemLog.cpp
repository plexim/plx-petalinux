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

#include <QtCore/QFile>

#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include "SystemLog.h"
#include "ImportantMessage.h"

#define DOMAIN_SOCKET_PATH "/tmp/display_log_socket"

SystemLog::SystemLog(ImportantMessage *aImportantMessageWidget, QWidget *aParent)
: QTextEdit(aParent),
  mImportantMessageWidget(aImportantMessageWidget)
{
    setReadOnly(true);
    /*
    setHtml("Lorem ipsum <b>dolor</b> sit amet, consectetur adipiscing elit. Aliquam dapibus tincidunt tempor."
    "Cras ac risus luctus, interdum erat eget, condimentum ligula. Nullam vitae ipsum magna. Nam dictum nec mauris"
    " sed feugiat. Nulla non ante finibus, pellentesque magna iaculis, condimentum nisi. Integer fringilla fermentum"
    " purus in mattis. Sed vitae dictum augue, et egestas lacus. Etiam euismod eros quam, nec pretium velit consectetur"
    " eget. Nunc a leo vulputate, tempus purus ac, vulputate mi.");
    */

    removeSocketFile();

    mLocalServer = new QLocalServer(this);
    connect(mLocalServer, &QLocalServer::newConnection, this, &SystemLog::acceptLocalServerConnection);
    mLocalServer->listen(DOMAIN_SOCKET_PATH);
}


void SystemLog::removeSocketFile()
{
    QFile socketFile(DOMAIN_SOCKET_PATH);
    if (socketFile.exists())
        socketFile.remove();
}


void SystemLog::acceptLocalServerConnection()
{
    auto *socket = mLocalServer->nextPendingConnection();
    if (!socket)
        return;

    connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
    connect(socket, &QLocalSocket::readyRead, [this, socket] () {
        this->processDataFromSocket(socket);
    });
}


void SystemLog::processDataFromSocket(QLocalSocket *aSocket)
{
    static const QLatin1String importantMessagePrefix("/PLX_IMPORTANT_MESSAGE/");

    auto data = QString(aSocket->readAll());

    if (data.startsWith(importantMessagePrefix))
    {
        data.remove(0, importantMessagePrefix.size());
        mImportantMessageWidget->newMessage(data);
    }
    else
    {
        append(data);
    }
}
