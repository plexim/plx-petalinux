#
# This file is the hil-config recipe.
#

SUMMARY = "Simple hil-config application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://hil_config.sh \
           file://rtbox.service \
           file://hil_firmware.sh \
           file://run_autostart.pl \
           file://mount_plexim.sh \
           file://automount_plexim.rules \
		  "

S = "${WORKDIR}"

RDEPENDS_${PN} += "perl"
RDEPENDS_${PN} += "perl-module-io-socket-unix"

do_install() {
        install -d ${D}/etc/init.d
        install -d ${D}/etc/rcS.d
        install -d ${D}/etc/rc5.d
        install -d ${D}/etc/avahi/services
        install -d ${D}/usr/bin
        install -m 0755 ${S}/hil_config.sh ${D}/etc/init.d
        install -m 0644 ${S}/rtbox.service ${D}/etc/avahi/services
        ln -s /etc/init.d/hil_config.sh ${D}/etc/rcS.d/S08hil_config.sh
        install -d ${D}${sysconfdir}/udev/mount.blacklist.d
        echo "/dev/mmcblk*" > ${D}${sysconfdir}/udev/mount.blacklist.d/plexim
	install -m 0755 ${S}/hil_firmware.sh ${D}/etc/init.d/hil_firmware.sh
	install -m 0755 ${S}/run_autostart.pl ${D}/usr/bin/run_autostart.pl
	ln -s /etc/init.d/hil_firmware.sh ${D}/etc/rc5.d/S90hil_firmware.sh
	install -d ${D}${sysconfdir}/udev/scripts
	install -d ${D}${sysconfdir}/udev/rules.d
        install -m 0755 ${S}/mount_plexim.sh ${D}${sysconfdir}/udev/scripts/mount_plexim.sh
        install -m 0644 ${S}/automount_plexim.rules ${D}${sysconfdir}/udev/rules.d/automount_plexim.rules
}
