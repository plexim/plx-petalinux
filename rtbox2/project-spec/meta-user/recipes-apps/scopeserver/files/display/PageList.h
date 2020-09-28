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

#ifndef PAGELIST_H
#define PAGELIST_H

#include <QtWidgets/QListWidget>


class PageList : public QListWidget
{
    Q_OBJECT

public:
    PageList(QWidget *aParent = nullptr);

    void setCurrentPage(QWidget *aPage);

signals:
    void activatePageRequest(QWidget *aPage);

protected:
    void keyPressEvent(QKeyEvent *aEvent) override;
};

#endif // PAGELIST_H
