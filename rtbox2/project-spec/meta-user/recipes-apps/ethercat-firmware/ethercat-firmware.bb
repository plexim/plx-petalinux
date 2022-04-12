#
# This file is the ethercat-firmware recipe.
#

SUMMARY = "Simple ethercat-firmware application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS_prepend := "${THISDIR}/../../../../../../vivado/rtbox_2/rtbox2_sdk/esf_r5/Release:"
SRC_URI = "file://esf_r5.elf"

FILES_${PN} += "/lib/firmware"
INSANE_SKIP_${PN} += "arch"

S = "${WORKDIR}"

do_install() {
	     install -d ${D}/lib/firmware
	     install -m 0644 ${S}/esf_r5.elf ${D}/lib/firmware
}
