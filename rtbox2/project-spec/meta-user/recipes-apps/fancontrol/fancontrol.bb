#
# This file is the fancontrol recipe.
#

SUMMARY = "Simple fancontrol application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://fancontrol \
	   file://fancontrol.sh \
		  "

S = "${WORKDIR}"

do_install() {
	     install -d ${D}${bindir}
             install -d ${D}${sysconfdir}/init.d
             install -d ${D}${sysconfdir}/rcS.d
	     install -m 0755 fancontrol ${D}${bindir}
             install -m 0755 fancontrol.sh ${D}${sysconfdir}/init.d
             ln -s /etc/init.d/fancontrol.sh /${D}${sysconfdir}/rcS.d/S66fancontrol.sh
}
