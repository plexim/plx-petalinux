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

#include "RTBoxOledBackingStore.h"
#include "RTBoxOledIntegration.h"
#include "qscreen.h"
#include <QLoggingCategory>
#include <qpa/qplatformscreen.h>
#include <private/qguiapplication_p.h>

#include <cstdint>

#include <metal/sys.h>

#include <rpumsg.h>
#include "display_backend_messages.h"


QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcRTBox, "qt.qpa.rtbox")


RTBoxOledBackingStore::RTBoxOledBackingStore(QWindow *window)
: QPlatformBackingStore(window)
{
    auto opt = RTBoxOledIntegration::instance()->options();

    m_debug = opt.debug;

    if (m_debug)
        qCDebug(qLcRTBox) << Q_FUNC_INFO << (quintptr)this;
}


RTBoxOledBackingStore::~RTBoxOledBackingStore()
{
}

QPaintDevice *RTBoxOledBackingStore::paintDevice()
{
    if (m_debug)
        qCDebug(qLcRTBox) << Q_FUNC_INFO;

    return &m_image;
}


static uint8_t rgb565_to_gray4(uint16_t pixel)
{
    uint16_t b = pixel & 0x1f;
    uint16_t g = (pixel & (0x3f << 5)) >> 5;
    uint16_t r = (pixel & (0x1f << (5 + 6))) >> (5 + 6);

    pixel = (299 * r + 587 * g + 114 * b) / 195;
    if (pixel > 255)
        pixel = 255;

    return (uint8_t)pixel >> 4;
}


static void setupUpdateMessagePointers(
    void *buffer,
    uint32_t *&msg_id,
    struct disp_back_msg_update_param *&par,
    uint8_t *&data)
{
    uint8_t *ptr = (uint8_t *)buffer;

    msg_id = (uint32_t *)ptr;
    ptr += 4;

    par = (struct disp_back_msg_update_param *)ptr;
    ptr += sizeof(struct disp_back_msg_update_param);

    data = ptr;
}


static void adjustUpdateRect(QRect &rect)
{
    uint32_t left = rect.left();
    uint32_t right = rect.right();

    left = (left & 0xFFFFFFFC);
    right = ((right + 4) & 0xFFFFFFFC) - 1;

    rect.setLeft(left);
    rect.setRight(right);
}


void RTBoxOledBackingStore::postUpdateRectMessage(const QRect &rect)
{
    const int MAX_IMAGE_DATA_SIZE = 256*64 / 2;
    const int PARAM_SIZE = sizeof(struct disp_back_msg_update_param);

    static uint8_t buffer[4 + PARAM_SIZE + MAX_IMAGE_DATA_SIZE];

    uint32_t *msg_id;
    struct disp_back_msg_update_param *par;
    uint8_t *data;

    setupUpdateMessagePointers(buffer, msg_id, par, data);

    QRect r = rect;

    adjustUpdateRect(r);

    if (m_debug)
        qCDebug(qLcRTBox) << Q_FUNC_INFO << r;

    *msg_id = DISP_BACK_MSG_UPDATE;

    par->bytes_per_row = r.width() / 2;
    par->top = r.top();
    par->left = r.left();
    par->bottom = r.bottom();
    par->right = r.right();

    const uint8_t *img8 = m_image.constBits();
    int data_idx = 0;

    if (m_image.format() == QImage::Format_RGB16)
    {
        img8 += (r.top() * m_image.bytesPerLine()) + (2 * r.left());

        for (int j = 0; j < r.height(); ++j)
        {
            uint16_t *img16 = (uint16_t *)img8;

            for (int i = 0; i < r.width()/2; i++)
            {
                uint8_t ph = rgb565_to_gray4(img16[i*2 + 0]);
                uint8_t pl = rgb565_to_gray4(img16[i*2 + 1]);

                data[data_idx] = (ph << 4) | pl;
                data_idx++;
            }

            img8 += m_image.bytesPerLine();
        }
    }
    else if (m_image.format() == QImage::Format_Grayscale8)
    {
        img8 += (r.top() * m_image.bytesPerLine()) + r.left();

        for (int j = 0; j < r.height(); ++j)
        {
            for (int i = 0; i < r.width()/2; i++)
            {
                uint8_t ph = img8[i*2 + 0];
                uint8_t pl = img8[i*2 + 1];

                data[data_idx] = (ph & 0xF0) | (pl >> 4);
                data_idx++;
            }

            img8 += m_image.bytesPerLine();
        }
    }

    int msg_size = 4 + PARAM_SIZE + data_idx;
    rpumsg_post_message(&buffer[0], msg_size);
}


void RTBoxOledBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (m_debug)
        qCDebug(qLcRTBox) << Q_FUNC_INFO << window << region << offset;

    for (const auto &rect: region)
    {
        postUpdateRectMessage(rect.translated(offset));
    }

    rpumsg_notify_remote();
}


void RTBoxOledBackingStore::resize(const QSize &size, const QRegion &)
{
    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();

    if (m_image.size() != size)
        m_image = QImage(size, format);

    if (m_debug)
        qCDebug(qLcRTBox) << Q_FUNC_INFO << size << format;
}


QT_END_NAMESPACE
