TARGET = rtbox_oled

TEMPLATE = lib
CONFIG += plugin

QT += \
    core-private \
    gui-private \
    eventdispatcher_support-private \
    fontdatabase_support-private \
    input_support-private

DEFINES += QT_NO_FOREACH

SOURCES =   main.cpp \
            RTBoxOledBackingStore.cpp \
            RTBoxOledIntegration.cpp

HEADERS =   \
    RTBoxOledBackingStore.h \
    RTBoxOledIntegration.h

OTHER_FILES += \
    rtbox_oled.json

INCLUDEPATH += ../../../../../../../xsdk/rtbox2/rpu/display_cairo_test_r5/src

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QMinimalIntegrationPlugin

LIBS += -lrpumsg -lmetal

target.path = /usr/lib/qt5/plugins/platforms
INSTALLS += target
