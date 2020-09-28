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

#ifndef RTBOX_OLED_BACKINGSTORE_H
#define RTBOX_OLED_BACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <qpa/qplatformwindow.h>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

struct rpumsg;

class RTBoxOledBackingStore : public QPlatformBackingStore
{
public:
    RTBoxOledBackingStore(QWindow *window);
    ~RTBoxOledBackingStore();

    QPaintDevice *paintDevice() Q_DECL_OVERRIDE;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) Q_DECL_OVERRIDE;
    void resize(const QSize &size, const QRegion &staticContents) Q_DECL_OVERRIDE;

protected:
    void postUpdateRectMessage(const QRect &rect);

private:
    QImage m_image;
    bool m_debug;
};

QT_END_NAMESPACE

#endif
