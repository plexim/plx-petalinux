QT += gui widgets
QT += network xml
CONFIG += debug_and_release c++11
SOURCES += main.cpp Server.cpp IOHelper.cpp SimulationRPC.cpp LocalServer.cpp LocalServerConnection.cpp ReleaseInfo.cpp \
    CanHandler.cpp \
    RPCReceiver.cpp \
    UdpTxHandler.cpp \
    UdpRxHandler.cpp \
    ToFileHandler.cpp \
    UnixSignalHandler.cpp \
    spinlock.S \
    FanControl.cpp \
    PerformanceCounter.cpp \
    FileWriter.cpp

HEADERS += Server.h IOHelper.h SimulationRPC.h LocalServer.h LocalServerConnection.h ReleaseInfo.h \
    CanHandler.h \
    RPCReceiver.h \
    UdpTxHandler.h \
    UdpRxHandler.h \
    ToFileHandler.h \
    UnixSignalHandler.h \
    spinlock.h \
    FanControl.h \
    PerformanceCounter.h \
    FileWriter.h

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

HEADERS += \
   display/Display.h \
   display/PageList.h \
   display/DisplayProxyStyle.h \
   display/SimulationInfo.h \
   display/CpuLoad.h \
   display/ElidedLabel.h \
   display/SystemLog.h \
   display/SimulationMessages.h \
   display/ImportantMessage.h

SOURCES += \
   display/Display.cpp \
   display/PageList.cpp \
   display/DisplayProxyStyle.cpp \
   display/SimulationInfo.cpp \
   display/CpuLoad.cpp \
   display/ElidedLabel.cpp \
   display/SystemLog.cpp \
   display/SimulationMessages.cpp \
   display/ImportantMessage.cpp

LIBS += -lmetal -lrpumsg
INCLUDEPATH += ../../../../../../../xsdk/rtbox2/rpu/display_backend/src
INCLUDEPATH += ../../../../../../../plexim_hil/rtbox2/src/plexim
INCLUDEPATH += ../../../../../components/plnx_workspace/fsbl/fsbl_bsp/psu_cortexa53_0/include

RESOURCES += \
   display/display.qrc

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


