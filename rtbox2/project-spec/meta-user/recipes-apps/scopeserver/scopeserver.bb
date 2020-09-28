#
# This file is the scopeserver recipe.
#

SUMMARY = "Simple scopeserver application"
SECTION = "PETALINUX/apps"
LICENSE = "GPLv3"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-3.0;md5=c79ff39f19dfec6d293b95dea7b07891"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:${THISDIR}/../../../../../../xsdk/rtbox2/rpu/display_backend/src:\
${THISDIR}/../../../../../../plexim_hil/rtbox2/src/plexim:\
${THISDIR}/../../../../components/plnx_workspace/fsbl/fsbl_bsp/psu_cortexa53_0/include:"

SRC_URI = "file://UdpRxHandler.cpp \
file://Server.cpp \
file://xml-rpc \
file://xml-rpc/README \
file://xml-rpc/RtBoxXmlRpcServer.h \
file://xml-rpc/maiaFault.cpp \
file://xml-rpc/LICENSE \
file://xml-rpc/maiaObject.cpp \
file://xml-rpc/maiaXmlRpcServerConnection.h \
file://xml-rpc/maiaXmlRpcServerConnection.cpp \
file://xml-rpc/maiaXmlRpcServer.h \
file://xml-rpc/maiaObject.h \
file://xml-rpc/maiaFault.h \
file://xml-rpc/RTBoxError.h \
file://xml-rpc/maiaXmlRpcServer.cpp \
file://xml-rpc/RtBoxXmlRpcServer.cpp \
file://LocalServerConnection.cpp \
file://main.cpp \
file://ReleaseInfo.cpp \
file://CanHandler.cpp \
file://scopeserver.sh \
file://LocalServer.cpp \
file://RPCReceiver.cpp \
file://LocalServerConnection.h \
file://UdpTxHandler.cpp \
file://IOHelper.h \
file://ReleaseInfo.h \
file://UdpTxHandler.h \
file://CanHandler.h \
file://IOHelper.cpp \
file://SimulationRPC.cpp \
file://Server.h \
file://UdpRxHandler.h \
file://LocalServer.h \
file://RPCReceiver.h \
file://SimulationRPC.h \
file://spinlock.h \
file://spinlock.S \
file://FanControl.h \
file://FanControl.cpp \
file://UnixSignalHandler.h \
file://UnixSignalHandler.cpp \
file://PerformanceCounter.h \
file://PerformanceCounter.cpp \
file://scopeserver.pro \
file://display/Display.h \
file://display/Display.cpp \
file://display_backend_messages.h;subdir=display \
file://display_backend_parameters.h \
file://display/SystemLog.h \
file://display/SystemLog.cpp \
file://display/oled.qss \
file://display/display.qrc \
file://display/DisplayProxyStyle.h \
file://display/DisplayProxyStyle.cpp \
file://display/PageList.h \
file://display/PageList.cpp \
file://display/SimulationInfo.h \
file://display/SimulationInfo.cpp \
file://display/CpuLoad.h \
file://display/CpuLoad.cpp \
file://display/ElidedLabel.h \
file://display/ElidedLabel.cpp \
file://display/SimulationMessages.h \
file://display/SimulationMessages.cpp \
file://display/ImportantMessage.h \
file://display/ImportantMessage.cpp \
file://hw_wrapper.h;subdir=display \
file://xparameters.h;subdir=xml-rpc \
file://xparameters_ps.h;subdir=xml-rpc \
		  "

S = "${WORKDIR}"

require recipes-qt/qt5/qt5.inc

# Add dependency packages
DEPENDS += "qtbase libxslt libsocketcan fsbl"

RDEPENDS_${PN} += "${@bb.utils.contains('PACKAGECONFIG_OPENSSL', 'openssl', 'ca-certificates', '', d)}"

DEPENDS += "libmetal librpumsg"
RDEPENDS_${PN} += "qt5-platform-plugin-oled"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 scopeserver ${D}${bindir}
             install -d ${D}/etc/init.d
             install -m 0755 ${S}/scopeserver.sh ${D}/etc/init.d/
             install -d ${D}/etc/rcS.d
	     ln -s /etc/init.d/scopeserver.sh ${D}/etc/rcS.d/S72scopeserver.sh
}
