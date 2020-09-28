#
# This file is the perl-cgi recipe.
#

SUMMARY = "Simple perl-cgi application"
SECTION = "PETALINUX/libs"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=415fc49abed2728f9480cd32c8d67beb"
SRC_URI[md5sum] = "2cbe560fdadbb8b9237744e39bbfc3eb"

inherit cpan

SRC_URI = "https://cpan.metacpan.org/authors/id/L/LE/LEEJO/CGI-4.44.tar.gz"

S = "${WORKDIR}/CGI-4.44"

BBCLASSEXTEND = "native"


