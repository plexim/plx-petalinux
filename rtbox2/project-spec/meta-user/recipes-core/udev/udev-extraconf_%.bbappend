SRC_URI += "file://10-display-buttons.rules"

do_install_append() {
	install -d ${D}${sysconfdir}/udev/rules.d
	install -m 0644 ${WORKDIR}/10-display-buttons.rules ${D}${sysconfdir}/udev/rules.d
}


FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

