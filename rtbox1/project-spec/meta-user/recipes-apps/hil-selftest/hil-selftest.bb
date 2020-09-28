#
# This file is the hil-selftest recipe.
#

SUMMARY = "Simple hil-selftest application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://test.html \
           file://boxtest.pl \
           file://js/spin.min.js \
           file://js/angular-spinner.min.js \
           file://js/rtboxtest.js \
		  "
DEPENDS = "lighttpd spawn-fcgi"
RDEPENDS_${PN} = "perl"
RDEPENDS_${PN} += "perl-module-time-hires"

FILES_${PN} += "/www/pages/js /www/pages/cgi-bin /www/cgi /www/pages"

S = "${WORKDIR}"

do_install() {
	install -d ${D}/www/cgi
	install -d ${D}/www/pages/cgi-bin
	touch ${D}/www/pages/cgi-bin/boxtest.cgi
	touch ${D}/www/pages/cgi-bin/teststatus.cgi
	install -d ${D}/www/pages/js
        cp -R ${S}/js/* ${D}/www/pages/js/
        install -m 0755 ${S}/boxtest.pl ${D}/www/cgi
        install -m 0644 ${S}/test.html ${D}/www/pages/test.html
}

