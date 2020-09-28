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

#ifndef UNIXSIGNALHANDLER_H
#define UNIXSIGNALHANDLER_H

#include <QtCore/QObject>

class QSocketNotifier;

class UnixSignalHandler: public QObject
{
    Q_OBJECT

public:
    UnixSignalHandler(QObject *aParent = nullptr);

    static void signalHandler(int aSignal);

public slots:
    void handleSignal();

private:
    static int mSocketFd[2];

    QSocketNotifier *mSocketNotifier;
};

#endif // UNIXSIGNALHANDLER_H
