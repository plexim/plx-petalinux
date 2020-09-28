#
# This file is the qt5-platform-plugin-oled recipe.
#

SUMMARY = "Qt5 platform plugin with support for the RT Box oled display."
SECTION = "libs"
LICENSE = "GPLv3"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-3.0;md5=c79ff39f19dfec6d293b95dea7b07891"

FILESEXTRAPATHS_prepend := "${THISDIR}/../../../../../../xsdk/rtbox2/rpu/display_cairo_test_r5/src:"

SRC_URI = " \
	file://rtbox_oled.pro \
	file://rtbox_oled.json \
        file://main.cpp \
	file://RTBoxOledIntegration.h \
	file://RTBoxOledIntegration.cpp \
	file://RTBoxOledBackingStore.h \
	file://RTBoxOledBackingStore.cpp \
	file://display_backend_messages.h \
	  "

S = "${WORKDIR}"

require recipes-qt/qt5/qt5.inc

DEPENDS += "qtbase librpumsg libmetal" 

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${libdir}/qt5/plugins/platforms
	     install -m 0755 librtbox_oled.so ${D}${libdir}/qt5/plugins/platforms
}

FILES_${PN} = "${libdir}/qt5/plugins/platforms/librtbox_oled.so"
FILES_${PN}-dev = ""

