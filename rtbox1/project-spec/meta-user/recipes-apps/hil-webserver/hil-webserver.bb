#
# This file is the hil-webserver recipe.
#

SUMMARY = "Simple hil-webserver application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:\
${THISDIR}/../../../../../common/angular:"

SRC_URI = "file://rtbox-webserver/ \
	file://Makefile \
	file://cgihandler.pl \
	file://cgi-bin/stop.cgi \
	file://cgi-bin/start.cgi \
	file://cgi-bin/upload.cgi \
        file://cgi-bin/front-panel.cgi \
        file://cgi-bin/netstate.cgi \
        file://cgi-bin/ipstate.cgi \
        file://private/syslog.cgi \
        file://private/scopeserverlog.cgi \
        file://js/webdav-min.js \
        file://css/webdavstyle-min.css \
        file://cgihandler \
        file://lib/dirheader.html \
        "



DEPENDS = "lighttpd spawn-fcgi"
RDEPENDS_${PN} += "perl"
RDEPENDS_${PN} += "perl-module-file-basename"
RDEPENDS_${PN} += "perl-module-file-copy"
RDEPENDS_${PN} += "perl-module-json-pp"
RDEPENDS_${PN} += "perl-module-io-socket-unix"
RDEPENDS_${PN} += "perl-module-fcntl"
RDEPENDS_${PN} += "perl-module-errno"

FILES_${PN} += "/www/cgi /www/pages /www/pages/js /www/pages/css /www/pages/dav/usb /www/lib /www/private"

S = "${WORKDIR}"

do_configure() {
}

do_compile() {           
             oe_runmake PETALINUX=${PETALINUX}
}

do_install() {
	install -d ${D}/www/cgi
        install -d ${D}/www/pages/js
        install -d ${D}/www/pages/dav/usb
        install -d ${D}/www/lib
        cp -R ${S}/js/* ${D}/www/pages/js/
	cp -R ${S}/css ${D}/www/pages
	cp -R ${S}/cgi-bin ${D}/www/pages
	cp -R ${S}/private ${D}/www
	install -c ${S}/cgihandler.pl ${D}/www/cgi
	install -d ${D}/var/sock
	install -d ${D}/etc/init.d
	install -m 0755 ${S}/cgihandler ${D}/etc/init.d
        install -d ${D}/etc/rcS.d
	ln -s /etc/init.d/cgihandler ${D}/etc/rcS.d/S71cgihandler
	cp -R ${S}/rtbox-webserver/rtbox1/* ${D}/www/pages
	install ${S}/lib/dirheader.html ${D}/www/lib
}
