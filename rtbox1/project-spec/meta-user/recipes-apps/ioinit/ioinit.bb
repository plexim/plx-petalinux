#
# This file is the ioinit recipe.
#

SUMMARY = "Simple ioinit application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ioinit.sh \
           file://versionInit.pl \
           file://40plexim \
	   file://Makefile \
		  "

S = "${WORKDIR}"

RDEPENDS_${PN} = "perl"

do_compile() {
	     oe_runmake
}

do_install() {
     install -d ${D}/etc/init.d
     install -d ${D}/etc/rcS.d
     install -d ${D}/etc/udhcpc.d
     install -m 0755 ioinit.sh ${D}/etc/init.d
     install -m 0755 versionInit.pl ${D}/etc/init.d
     install -m 0755 40plexim ${D}/etc/udhcpc.d/40plexim
     ln -s /etc/init.d/ioinit.sh /${D}/etc/rcS.d/S07ioinit.sh	
}
