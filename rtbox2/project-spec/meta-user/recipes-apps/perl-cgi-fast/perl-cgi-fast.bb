#
# This file is the perl-cgi-fast recipe.
#

SUMMARY = "Simple perl-cgi-fast application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
SRC_URI[md5sum] = "f9dac0982ccce4a9b89433b378b1573d"

inherit cpan

SRC_URI = "https://cpan.metacpan.org/authors/id/L/LE/LEEJO/CGI-Fast-2.15.tar.gz"

S = "${WORKDIR}/CGI-Fast-2.15"

BBCLASSEXTEND = "native"

