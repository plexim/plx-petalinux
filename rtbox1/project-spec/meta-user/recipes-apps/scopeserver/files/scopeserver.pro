QT -= gui
QT += network xml
CONFIG += debug_and_release c++11
SOURCES += main.cpp Server.cpp IOHelper.cpp SimulationRPC.cpp LocalServer.cpp LocalServerConnection.cpp ReleaseInfo.cpp \
    CanHandler.cpp \
    RPCReceiver.cpp \
    UdpTxHandler.cpp \
    UdpRxHandler.cpp \
    FanControl.cpp

HEADERS += Server.h IOHelper.h SimulationRPC.h LocalServer.h LocalServerConnection.h ReleaseInfo.h \
    CanHandler.h \
    RPCReceiver.h \
    UdpTxHandler.h \
    UdpRxHandler.h \
    FanControl.h \
    xparameters.h \
    xparameters_ps.h

HEADERS += xml-rpc/maiaFault.h
SOURCES += xml-rpc/maiaFault.cpp
HEADERS += xml-rpc/maiaObject.h
SOURCES += xml-rpc/maiaObject.cpp
HEADERS += xml-rpc/maiaXmlRpcServer.h
SOURCES += xml-rpc/maiaXmlRpcServer.cpp
HEADERS += xml-rpc/maiaXmlRpcServerConnection.h
SOURCES += xml-rpc/maiaXmlRpcServerConnection.cpp
HEADERS += xml-rpc/RtBoxXmlRpcServer.h
SOURCES += xml-rpc/RtBoxXmlRpcServer.cpp

# QMAKE_POST_LINK += $${QMAKE_STRIP} $(TARGET)
GIT_REV = $$system(touch ReleaseInfo.cpp && git describe --tags --always)
BUILD_DATE = $$system(date \"+%d.%m.%Y %H:%M\")
QMAKE_CXXFLAGS += -DBUILD_DATE=\"$${BUILD_DATE}\" -DGIT_REV=\"$${GIT_REV}\"
target.path = /bin
contains(BUILD_ENV, petalinux) {
   LIBS += -L../../stage/usr/lib -lsocketcan
   INCLUDEPATH += ../../stage/usr/include
} else {
   LIBS += -L../../../build/linux/rootfs/stage/usr/lib -lsocketcan
   INCLUDEPATH += ../../../build/linux/rootfs/stage/usr/include
}
INSTALLS += target
