#
# This file is the display-cairo-test recipe.
#

SUMMARY = "Simple display-cairo-test application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS += "cairomm libmetal librpumsg"

FILESEXTRAPATHS_prepend := "${THISDIR}/../../../../../../xsdk/rtbox2/rpu/display_cairo_test_r5/src:"

SRC_URI = "file://CMakeLists.txt \
           file://main.cpp \
	   file://cairo_demo.cpp \
	   file://cairo_demo.h \
           file://display_backend_messages.h \
	  "
inherit pkgconfig cmake

S = "${WORKDIR}"

