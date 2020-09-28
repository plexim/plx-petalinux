SUMMARY = "Jailhouse"
HOMEPAGE = "https://github.com/siemens/jailhouse"
SECTION = "jailhouse"
LICENSE = "GPL-2.0 & BSD-2-Clause"

LIC_FILES_CHKSUM = "file://COPYING;md5=9fa7f895f96bde2d47fd5b7d95b6ba4d \
                 file://tools/root-cell-config.c.tmpl;beginline=6;endline=33;md5=2825581c1666c44a17955dc574cfbfb3 \
                 file://include/jailhouse/hypercall.h;beginline=9;endline=36;md5=2825581c1666c44a17955dc574cfbfb3 \
                 file://include/jailhouse/cell-config.h;beginline=9;endline=36;md5=2825581c1666c44a17955dc574cfbfb3 \
                 file://include/arch/arm/asm/jailhouse_hypercall.h;beginline=9;endline=36;md5=2825581c1666c44a17955dc574cfbfb3 \
                 file://include/arch/arm64/asm/jailhouse_hypercall.h;beginline=9;endline=36;md5=2825581c1666c44a17955dc574cfbfb3 \
                 file://include/arch/x86/asm/jailhouse_hypercall.h;beginline=9;endline=36;md5=2825581c1666c44a17955dc574cfbfb3 \
                 file://driver/jailhouse.h;beginline=9;endline=36;md5=2825581c1666c44a17955dc574cfbfb3"

DEPENDS = "virtual/kernel make-native python3-mako-native dtc-native"

S ="${WORKDIR}/git"

require jailhouse-defs.inc
inherit module pythonnative kernel-arch

JH_CONFIG ?= "${S}/ci/jailhouse-config.h"
JH_CONFIG_x86 ?= "${S}/ci/jailhouse-config.h"
JH_CONFIG_x86-64 ?= "${S}/ci/jailhouse-config.h"
JH_CONFIG_arm ?= "${S}/ci/jailhouse-config.h"

do_configure() {
   if [ -d ${STAGING_DIR_HOST}/${CELLCONF_DIR} ]; 
   then
      rm ${S}/configs/arm64/*.c
      cp "${STAGING_DIR_HOST}/${CELLCONF_DIR}/"*.c ${S}/configs/arm64/
   fi

   if [ ! -z ${JH_CONFIG} ];
   then
      cp -av ${JH_CONFIG} ${S}/include/jailhouse/config.h
   fi
}

USER_SPACE_CFLAGS = '${CFLAGS} -DLIBEXECDIR=\\\"${libexecdir}\\\" \
		  -DJAILHOUSE_VERSION=\\\"$JAILHOUSE_VERSION\\\" \
		  -Wall -Wextra -Wmissing-declarations -Wmissing-prototypes -Werror \
		  -I../driver'

TOOLS_SRC_DIR = "${S}/tools"
TOOLS_OBJ_DIR = "${S}/tools"

do_compile() {
   oe_runmake V=0 \
        CROSS_COMPILE=${TARGET_PREFIX} LDFLAGS="" \
        KDIR=${STAGING_KERNEL_BUILDDIR}

   cd ${TOOLS_SRC_DIR}
   export JAILHOUSE_VERSION=$(cat ../VERSION)
   oe_runmake V=0 \
   	       CFLAGS="${USER_SPACE_CFLAGS}" \
               src=${TOOLS_SRC_DIR} obj=${TOOLS_OBJ_DIR} \
	       ${TOOLS_OBJ_DIR}/jailhouse
}

do_install() {
    oe_runmake \
	       CROSS_COMPILE=${TARGET_PREFIX} LDFLAGS="" \
	       KDIR=${STAGING_KERNEL_BUILDDIR} \
	       DESTDIR=${D} modules_install firmware_install

    cd ${TOOLS_SRC_DIR}
    oe_runmake \
   	       CFLAGS="${USER_SPACE_CFLAGS}" \
               src=${TOOLS_SRC_DIR} obj=${TOOLS_OBJ_DIR} \
	       DESTDIR=${D} install-bin install-libexec install-data

    install -d ${D}${CELL_DIR}
    install ${B}/configs/arm64/*.cell ${D}${CELL_DIR}/
}

PACKAGE_BEFORE_PN = "kernel-module-jailhouse"
FILES_${PN} = "${base_libdir}/firmware ${libexecdir} ${sbindir} ${JH_DATADIR}"

KERNEL_MODULE_AUTOLOAD += "jailhouse"

INSANE_SKIP_${PN} = "ldflags"

python __anonymous () {
    # Setup DEPENDS and RDEPENDS to included cells"
    cells = d.getVar('CELLS', True) or ""
    for cell in cells.split():
        d.appendVar('DEPENDS', ' ' + cell)
        d.appendVar('RDEPENDS_${PN}', ' ' + cell)
}

