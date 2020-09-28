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

#include "RTBoxOledIntegration.h"
#include "RTBoxOledBackingStore.h"

#include <QtGui/private/qpixmap_raster_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformwindow.h>

#if QT_CONFIG(fontconfig)
#  include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#  include <qpa/qplatformfontdatabase.h>
#endif

#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>

#include <qpa/qplatforminputcontextfactory_p.h>

#if QT_CONFIG(evdev)
#  include <QtInputSupport/private/qevdevkeyboardmanager_p.h>
#endif

QT_BEGIN_NAMESPACE


class DummyFontDatabase : public QPlatformFontDatabase
{
public:
    virtual void populateFontDatabase() Q_DECL_OVERRIDE {}
};


RTBoxOledIntegration::Options RTBoxOledIntegration::parseOptions(const QStringList &paramList)
{
    Options opt;

    for (const QString &param : paramList)
    {
        auto tokens = param.split("=");
        auto key = tokens.first();
        auto value = tokens.last();

        if (key == QLatin1String("debug"))
            opt.debug = true;
        else if (key == QLatin1String("nofonts"))
            opt.no_fonts = true;
        else if (key == QLatin1String("noinput"))
            opt.no_input = true;
        else if (key == QLatin1String("gray8_backingstore"))
            opt.gray8_backingstore = true;
    }

    return opt;
}


RTBoxOledIntegration::RTBoxOledIntegration(const QStringList &parameters)
: m_fontDatabase(nullptr)
, m_options(parseOptions(parameters))
{
}


RTBoxOledIntegration::~RTBoxOledIntegration()
{
    destroyScreen(m_primaryScreen);
    delete m_fontDatabase;
}


void RTBoxOledIntegration::initialize()
{
    /* screen */

    m_primaryScreen = new OledScreen();

    m_primaryScreen->mGeometry = QRect(0, 0, 256, 64);
    m_primaryScreen->mDepth = m_options.gray8_backingstore ? 8 : 16;
    m_primaryScreen->mFormat = m_options.gray8_backingstore ?  QImage::Format_Grayscale8 : QImage::Format_RGB16;

    screenAdded(m_primaryScreen);

    /* font database */

    if (!m_options.no_fonts)
    {
#if QT_CONFIG(fontconfig)
        m_fontDatabase = new QGenericUnixFontDatabase;
#endif
    }

    if (!m_fontDatabase)
    {
        m_fontDatabase = new DummyFontDatabase;
    }

    /* input context */

    m_inputContext = QPlatformInputContextFactory::create();

    if (!m_options.no_input)
    {
#if QT_CONFIG(evdev)
        new QEvdevKeyboardManager(QLatin1String("EvdevKeyboard"), QString(), this);
#endif
    }
}


bool RTBoxOledIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap)
    {
        case ThreadedPixmaps: return true;
        case MultipleWindows: return true;

        default:
            return QPlatformIntegration::hasCapability(cap);
    }
}


QPlatformFontDatabase *RTBoxOledIntegration::fontDatabase() const
{
    return m_fontDatabase;
}


QPlatformInputContext *RTBoxOledIntegration::inputContext() const
{
    return m_inputContext;
}


QPlatformWindow *RTBoxOledIntegration::createPlatformWindow(QWindow *window) const
{
    Q_UNUSED(window);

    QPlatformWindow *w = new QPlatformWindow(window);
    w->requestActivateWindow();

    return w;
}


QPlatformBackingStore *RTBoxOledIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new RTBoxOledBackingStore(window);
}


QAbstractEventDispatcher *RTBoxOledIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}


RTBoxOledIntegration *RTBoxOledIntegration::instance()
{
    return static_cast<RTBoxOledIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QT_END_NAMESPACE
