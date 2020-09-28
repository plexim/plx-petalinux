QT += widgets

SOURCES = main.cpp

INCLUDEPATH += ../../../../../../../xsdk/rtbox2/rpu/display_cairo_test_r5/src

LIBS += -lrpumsg -lmetal

target.path = /usr/bin
INSTALLS += target
