#
# This file is the perl-fcgi recipe.
#

SUMMARY = "Simple perl-fcgi application"
SECTION = "PETALINUX/libs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e3aacac3a647af6e7e31f181cda0a06a"
SRC_URI[md5sum] = "916cd2887b27265cd8dcfd3280135270"

inherit cpan
#inherit siteconfig

SRC_URI = "https://cpan.metacpan.org/authors/id/E/ET/ETHER/FCGI-0.78.tar.gz"

S = "${WORKDIR}/FCGI-0.78"
inherit cpan

BBCLASSEXTEND = "native"

do_configure() {
	cpan_do_configure
	./configure --host ${HOST_ARCH}
}

