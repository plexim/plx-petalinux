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

#include <QtGui/QPainter>

#include "SimulationMessages.h"

SimulationMessages::SimulationMessages(QWidget *aParent)
: QTextEdit(aParent)
{
    setReadOnly(true);
}


void SimulationMessages::paintEvent(QPaintEvent *aEvent)
{
    if (document()->isEmpty())
    {
        QPainter painter(viewport());
        QRect rect(viewport()->contentsRect());
        painter.drawText(rect, Qt::AlignCenter, "(empty)");
    }
    else
    {
        QTextEdit::paintEvent(aEvent);
    }
}
