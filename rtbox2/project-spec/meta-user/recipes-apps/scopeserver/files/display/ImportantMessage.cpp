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

#include <QtGui/QKeyEvent>

#include <QtWidgets/QLabel>

#include "ImportantMessage.h"
#include "Display.h"

ImportantMessage::ImportantMessage(Display *aDisplay, QWidget *aParent)
: QScrollArea(aParent),
  mDisplay(aDisplay),
  mPreviouslyActiveWidget(nullptr)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    mMessageLabel = new ImportantMessageText;
    mMessageLabel->setAlignment(Qt::AlignHCenter);
    mMessageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    mMessageLabel->setWordWrap(true);
    setWidget(mMessageLabel);
}


void ImportantMessage::newMessage(const QString &aMessage)
{
    mMessageLabel->setText(aMessage);
    mMessageLabel->adjustSize();

    auto currentWidget = mDisplay->currentWidget();

    if (currentWidget != this)
    {
        mPreviouslyActiveWidget = currentWidget;
        mDisplay->activateWidget(this);
    }

    mDisplay->pokeBackend();
}


void ImportantMessage::closeMessage()
{
    if (mDisplay->currentWidget() == this)
    {
        mDisplay->activateWidget(mPreviouslyActiveWidget);
    }
}


void ImportantMessage::keyPressEvent(QKeyEvent *aEvent)
{
    if (aEvent->key() == Qt::Key_Return ||
        aEvent->key() == Qt::Key_Backspace )
    {
        closeMessage();
    }
    else
    {
        QScrollArea::keyPressEvent(aEvent);
    }
}
