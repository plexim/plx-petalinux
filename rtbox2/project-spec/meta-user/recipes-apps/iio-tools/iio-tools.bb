SUMMARY = "iio tools"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0;md5=801f80980d171dd6425610833a22dbe6"
SRC_URI = "file://iio_event_monitor.c \
           file://iio_generic_buffer.c \
	   file://iio_utils.c \
	   file://iio_utils.h \
	   file://lsiio.c \
           file://Makefile \
          "
S = "${WORKDIR}"
CFLAGS_prepend = "-D_GNU_SOURCE"

do_compile() {
        oe_runmake
}

do_install() {
        install -d ${D}${bindir}
        install -m 0755 ${S}/lsiio ${D}${bindir}
        install -m 0755 ${S}/iio_event_monitor ${D}${bindir}
        install -m 0755 ${S}/iio_generic_buffer ${D}${bindir}

}

