FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://index.html \
            file://fastcgi.conf \
"

do_install_append() {
	cp ${S}/../index.html ${D}/www/pages
	cp ${S}/../fastcgi.conf ${D}/etc/lighttpd.d
}


