/*
 * Copyright (c) 2015-2020 Plexim GmbH.
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

#include "ReleaseInfo.h"

#define VERSION 2002.001

#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)

const double ReleaseInfo::SCOPESERVER_VERSION = VERSION * VERSION;
const QString ReleaseInfo::revisionInfo = QString(QUOTEME(GIT_REV)) + "  " + QUOTEME(BUILD_DATE);

