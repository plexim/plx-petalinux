SUMMARY = "Root cell for Jailhouse on RT Box"
LICENSE = "Plexim-1.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=49cdac8baef387df7b8f108d8edb175c"

SRC_URI = "file://rtbox-root.c \
           file://LICENSE \
          "

S = "${WORKDIR}"

inherit jailhouse-cell

CELLCONFIG = "${S}/rtbox-root.c"
ALLOW_EMPTY_${PN} = "1"

