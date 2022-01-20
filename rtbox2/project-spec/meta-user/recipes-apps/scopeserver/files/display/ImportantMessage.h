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

#ifndef IMPORTANTMESSAGE_H
#define IMPORTANTMESSAGE_H

#include <QtWidgets/QScrollArea>
#include <QtWidgets/QLabel>

class Display;


class ImportantMessageText: public QLabel
{
    Q_OBJECT
};


class ImportantMessage: public QScrollArea
{
    Q_OBJECT

public:
    ImportantMessage(Display *aDisplay, QWidget *aParent = nullptr);

public slots:
    void newMessage(const QString &aMessage);
    void closeMessage();

protected:
    void keyPressEvent(QKeyEvent *aEvent) override;

private:
    Display *mDisplay;
    QWidget *mPreviouslyActiveWidget;
    ImportantMessageText *mMessageLabel;
};

#endif // IMPORTANTMESSAGE_H
