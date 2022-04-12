#
# This file is the power-scripts recipe.
#

SUMMARY = "Simple power-scripts application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://read-ps-temp.sh \
	   file://read-power.sh \
	"

S = "${WORKDIR}"

do_install() {
	     install -d ${D}/${bindir}
	     install -m 0755 ${S}/read-ps-temp.sh ${D}/${bindir}
	     install -m 0755 ${S}/read-power.sh ${D}/${bindir}
}

