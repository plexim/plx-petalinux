#
# This file is the display-qt-test recipe.
#

SUMMARY = "Simple test for the Qt oled platform plugin."
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS_prepend := "${THISDIR}/../../../../../../xsdk/rtbox2/rpu/display_cairo_test_r5/src:"

SRC_URI = " \
	file://display-qt-test.pro \
	file://display-qt-test.sh \
        file://main.cpp \
	file://display_backend_messages.h \
	  "

S = "${WORKDIR}"

require recipes-qt/qt5/qt5.inc

DEPENDS += "qtbase libmetal librpumsg"
RDEPENDS_${PN} += "qt5-platform-plugin-oled" 

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 display-qt-test ${D}${bindir}/display-qt-test.bin
	     install -m 0755 ${S}/display-qt-test.sh ${D}${bindir}/display-qt-test
}

FILES_${PN} = "${bindir}/display-qt-test.bin ${bindir}/display-qt-test"

