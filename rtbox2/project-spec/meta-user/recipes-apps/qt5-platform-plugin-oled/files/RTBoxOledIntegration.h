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

#ifndef RTBOX_OLED_INTEGRATION_H
#define RTBOX_OLED_INTEGRATION_H

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

class OledScreen : public QPlatformScreen
{
public:
    OledScreen() {}

    QRect geometry() const Q_DECL_OVERRIDE { return mGeometry; }
    int depth() const Q_DECL_OVERRIDE { return mDepth; }
    QImage::Format format() const Q_DECL_OVERRIDE { return mFormat; }

public:
    QRect mGeometry;
    int mDepth;
    QImage::Format mFormat;
};


class RTBoxOledIntegration : public QPlatformIntegration, public QPlatformNativeInterface
{
public:
    struct Options {
        bool debug = false;
        bool no_fonts = false;
        bool no_input = false;
        bool gray8_backingstore = false;
    };

    explicit RTBoxOledIntegration(const QStringList &parameters);
    ~RTBoxOledIntegration();

    void initialize() Q_DECL_OVERRIDE;

    bool hasCapability(QPlatformIntegration::Capability cap) const Q_DECL_OVERRIDE;
    QPlatformFontDatabase *fontDatabase() const Q_DECL_OVERRIDE;
    QPlatformInputContext *inputContext() const Q_DECL_OVERRIDE;

    QPlatformWindow *createPlatformWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformBackingStore *createPlatformBackingStore(QWindow *window) const Q_DECL_OVERRIDE;
    QAbstractEventDispatcher *createEventDispatcher() const Q_DECL_OVERRIDE;

    Options options() const { return m_options; }
    static RTBoxOledIntegration *instance();

protected:
    Options parseOptions(const QStringList &paramList);

private:
    OledScreen *m_primaryScreen;
    QPlatformFontDatabase *m_fontDatabase;
    QPlatformInputContext *m_inputContext;

    Options m_options;
};

QT_END_NAMESPACE

#endif
