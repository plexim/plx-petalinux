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
//  ElidedLabel.h
//  plecseditor
//
//  Created by Wolfgang Hammer on 22/04/16.
//  Copyright (c) 2016 Plexim GmbH. All rights reserved.
//

#ifndef ELIDEDLABEL_H_
#define ELIDEDLABEL_H_

#include <QtWidgets/QFrame>

class ElidedLabel : public QFrame
{
public:
   ElidedLabel(QWidget *aParent = nullptr);
   
   void setText(const QString& aText);
   void setElideMode(Qt::TextElideMode aMode);
   
   virtual QSize sizeHint() const override;
   virtual QSize minimumSizeHint() const override;
   virtual void paintEvent(QPaintEvent *aEvent) override;
   
private:
   QString mText;
   Qt::TextElideMode mElideMode;
};

#endif /* defined(ELIDEDLABEL_H_) */
