#
# librpumsg
#

SUMMARY = "Library to send messages to the R5 firmware"
SECTION = "libs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS += "libmetal"

SRC_URI = "file://CMakeLists.txt \
	   file://rpumsg.c \
	   file://rpumsg.h \
	  "

inherit pkgconfig cmake

S = "${WORKDIR}"

