FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://00avahi-autoipd \
            file://99avahi-autoipd \
"

do_install_append() {
	cp ${S}/../00avahi-autoipd ${D}/etc/udhcpc.d
	cp ${S}/../99avahi-autoipd ${D}/etc/udhcpc.d
}


