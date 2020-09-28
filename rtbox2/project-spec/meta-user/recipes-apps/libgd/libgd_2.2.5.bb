#
# This file is the libgd recipe.
#

DEPENDS += "libpng zlib"
SUMMARY = "libgd library"
SECTION = "PETALINUX/apps"
LICENSE = "Libgd"
LIC_FILES_CHKSUM = "file://COPYING;md5=07384b3aa2e0d39afca0d6c40286f545"

SRC_URI = "https://github.com/${PN}/${PN}/releases/download/gd-${PV}/${PN}-${PV}.tar.gz"
SRC_URI[md5sum] = "ab2bd17470b51387eadfd5289b5c0dfb"
SRC_URI[sha256sum] = "a66111c9b4a04e818e9e2a37d7ae8d4aae0939a100a36b0ffb52c706a09074b5"

S = "${WORKDIR}/${PN}-${PV}"

inherit autotools binconfig-disabled pkgconfig

BBCLASSEXTEND = "native nativesdk"

PACKAGES += "${PN}-tools"

RDEPENDS_${PN}-tools = "perl"
FILES_${PN}-tools = "${bindir}/bdftogd"

