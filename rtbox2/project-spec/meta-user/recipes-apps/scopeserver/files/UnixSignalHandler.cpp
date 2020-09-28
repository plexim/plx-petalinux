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
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>

#include <QtCore/QSocketNotifier>
#include <QtCore/QCoreApplication>

#include "UnixSignalHandler.h"

int UnixSignalHandler::mSocketFd[2];


UnixSignalHandler::UnixSignalHandler(QObject *aParent)
: QObject(aParent)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, mSocketFd))
       qFatal("Couldn't create socketpair");

    mSocketNotifier = new QSocketNotifier(mSocketFd[1], QSocketNotifier::Read, this);
    connect(mSocketNotifier, &QSocketNotifier::activated, this, &UnixSignalHandler::handleSignal);

    struct sigaction action;

    action.sa_handler = UnixSignalHandler::signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_flags |= SA_RESTART;

    for (int sig: {SIGTERM, SIGHUP, SIGINT})
    {
        if (::sigaction(sig, &action, 0))
            qFatal("Couldn't set up signal handler");
    }
}


void UnixSignalHandler::signalHandler(int aSignal)
{
   ::write(mSocketFd[0], &aSignal, sizeof(int));
}


void UnixSignalHandler::handleSignal()
{
    mSocketNotifier->setEnabled(false);

    int sig;
    ::read(mSocketFd[1], &sig, sizeof(int));

    switch (sig)
    {
    case SIGTERM:
    case SIGINT:
    case SIGHUP:
        qApp->quit();
    }

    mSocketNotifier->setEnabled(true);
}
