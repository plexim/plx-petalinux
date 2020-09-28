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

#ifndef SYSTEMLOG_H
#define SYSTEMLOG_H

#include <QtWidgets/QTextEdit>

class QLocalServer;
class QLocalSocket;
class ImportantMessage;

class SystemLog : public QTextEdit
{
    Q_OBJECT

public:
    SystemLog(ImportantMessage *aImportantMessageWidget, QWidget *aParent = nullptr);

protected slots:
    void acceptLocalServerConnection();

protected:
    void removeSocketFile();
    void processDataFromSocket(QLocalSocket *aSocket);

private:
    QLocalServer *mLocalServer;
    ImportantMessage *mImportantMessageWidget;
};

#endif // SYSTEMLOG_H
