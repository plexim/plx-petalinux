FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://svg.conf \
            file://fastcgi.conf \
            file://logs.conf \
"

do_install_append() {
	cp ${S}/../svg.conf ${D}/etc/lighttpd.d
	cp ${S}/../fastcgi.conf ${D}/etc/lighttpd.d
	cp ${S}/../logs.conf ${D}/etc/lighttpd.d
}


