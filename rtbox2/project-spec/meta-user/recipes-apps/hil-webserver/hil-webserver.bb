#
# This file is the hil-webserver recipe.
#

SUMMARY = "Simple hil-webserver application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://favicon.ico \
	   file://css/rtbox.css \
	   file://css/bootstrap.min.css \
	   file://css/c3.css \
	   file://cgihandler.pl \
	   file://partials \
	   file://partials/analogIn.html \
	   file://partials/digitalIn.html \
	   file://partials/d-sub-37-male.html \
	   file://partials/d-sub-37-female.html \
	   file://partials/digitalOut.html \
	   file://partials/analogOut.html \
	   file://partials/ethernet.html \
	   file://cgi-bin/stop.cgi \
	   file://cgi-bin/start.cgi \
	   file://cgi-bin/upload.cgi \
           file://cgi-bin/front-panel.cgi \
           file://cgi-bin/netstate.cgi \
           file://cgi-bin/ipstate.cgi \
	   file://js/jquery.min.js \
	   file://js/c3.min.js \
	   file://js/d3.min.js \
	   file://js/ui-bootstrap-tpls-1.2.4.min.js \
	   file://js/ui-bootstrap-1.2.4.min.js \
	   file://js/xmlrpc.min.js \
	   file://js/c3-angular.js \
	   file://js/rtbox2.js \
	   file://js/angular.min.js \
           file://cgihandler \
		  "

DEPENDS = "lighttpd spawn-fcgi"
RDEPENDS_${PN} = "perl"

FILES_${PN} += "/www/pages/js /www/pages/css /www/pages/partials /www/cgi /www/pages"

S = "${WORKDIR}"

do_install() {
	install -d ${D}/www/cgi
	install -d ${D}/www/pages/js
        cp -R ${S}/js/* ${D}/www/pages/js/
	cp -R ${S}/css ${D}/www/pages
	cp -R ${S}/partials ${D}/www/pages
	cp -R ${S}/cgi-bin ${D}/www/pages
	cp ${S}/favicon.ico ${D}/www/pages
	install -c ${S}/cgihandler.pl ${D}/www/cgi
	install -d ${D}/var/sock
	install -d ${D}/etc/init.d
	install -m 0755 ${S}/cgihandler ${D}/etc/init.d
        install -d ${D}/etc/rcS.d
	ln -s /etc/init.d/cgihandler ${D}/etc/rcS.d/S71cgihandler
}
