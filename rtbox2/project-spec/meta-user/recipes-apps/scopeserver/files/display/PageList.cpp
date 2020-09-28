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

#include "PageList.h"


PageList::PageList(QWidget *aParent)
: QListWidget(aParent)
{
    //setFocusPolicy(Qt::NoFocus);
    setSelectionMode(QAbstractItemView::SingleSelection);
}


void PageList::keyPressEvent(QKeyEvent *aEvent)
{
    if (aEvent->key() == Qt::Key_Return)
    {
        auto *item = currentItem();
        if (item != nullptr)
        {
            auto *widget = static_cast<QWidget*>(item->data(Qt::UserRole).value<void*>());
            emit activatePageRequest(widget);
        }
    }
    else
    {
        QListWidget::keyPressEvent(aEvent);
    }
}


void PageList::setCurrentPage(QWidget *aPage)
{
    for (int row = 0; row < count(); ++row)
    {
        QListWidgetItem *rowItem = item(row);
        auto *widget = static_cast<QWidget*>(rowItem->data(Qt::UserRole).value<void*>());

        if (widget == aPage)
        {
            setCurrentRow(row);
            return;
        }
    }
}
