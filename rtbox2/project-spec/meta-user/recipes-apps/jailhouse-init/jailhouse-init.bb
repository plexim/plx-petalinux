#
# This file is the jailhouse recipe.
#

SUMMARY = "Simple jailhouse application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://jailhouse \
		  "

S = "${WORKDIR}"


do_install() {
	install -d ${D}/etc/init.d
        install -m 0755 ${S}/jailhouse ${D}/etc/init.d/
        install -d ${D}/etc/rcS.d
	ln -s /etc/init.d/jailhouse ${D}/etc/rcS.d/S70jailhouse
}
