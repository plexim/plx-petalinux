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

//
//  ElidedLabel.cpp
//  plecseditor
//
//  Created by Wolfgang Hammer on 22/04/16.
//  Copyright (c) 2016 Plexim GmbH. All rights reserved.
//

#include "ElidedLabel.h"
#include <QtGui/QPainter>

ElidedLabel::ElidedLabel(QWidget *aParent) : QFrame(aParent), mText(), mElideMode(Qt::ElideMiddle)
{
}

void ElidedLabel::setText(const QString& aText)
{
   mText = aText;
   setToolTip(aText);
   updateGeometry();
   update();
}

void ElidedLabel::setElideMode(Qt::TextElideMode aMode)
{
   mElideMode = aMode;
   updateGeometry();
   update();
}

QSize ElidedLabel::sizeHint() const
{
   const QFontMetrics &fm = fontMetrics();
   return QSize(fm.width(mText), fm.height());
}

QSize ElidedLabel::minimumSizeHint() const
{
   if (mElideMode == Qt::ElideNone)
      return sizeHint();
   const QFontMetrics &fm = fontMetrics();
   return QSize(fm.width("..."), fm.height());
}

void ElidedLabel::paintEvent(QPaintEvent *aEvent)
{
   QFrame::paintEvent(aEvent);
   QPainter p(this);
   QRect r(contentsRect());
   const QString elidedText = fontMetrics().elidedText(mText, mElideMode, r.width());
   p.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
}
