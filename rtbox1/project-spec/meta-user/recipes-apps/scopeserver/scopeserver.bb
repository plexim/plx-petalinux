#
# This file is the scopeserver recipe.
#

SUMMARY = "Simple scopeserver application"
SECTION = "PETALINUX/apps"
LICENSE = "GPLv3"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-3.0;md5=c79ff39f19dfec6d293b95dea7b07891"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:\
${THISDIR}/../../../../components/plnx_workspace/fsbl/fsbl_bsp/ps7_cortexa9_0/include:"

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
file://FanControl.cpp \
file://FanControl.h \
file://scopeserver.pro \
file://xparameters.h \
file://xparameters_ps.h \
		  "

S = "${WORKDIR}"

require recipes-qt/qt5/qt5.inc

do_compile[nostamp] ="1"

# Add dependency packages
DEPENDS += "qtbase libxslt libsocketcan"
do_fetch[depends] = "fsbl:do_compile"


RDEPENDS_${PN} += "${@bb.utils.contains('PACKAGECONFIG_OPENSSL', 'openssl', 'ca-certificates', '', d)}"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 scopeserver ${D}${bindir}
             install -d ${D}/etc/init.d
             install -m 0755 ${S}/scopeserver.sh ${D}/etc/init.d/
             install -d ${D}/etc/rc5.d
	     ln -s /etc/init.d/scopeserver.sh ${D}/etc/rc5.d/S72scopeserver.sh
}
