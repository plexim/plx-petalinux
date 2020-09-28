SUMMARY = "Init scripts for displaying log messages during boot."
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://displog.sh \
	   file://if-pre-up.sh \
	   file://if-up.sh \
	   file://udhcpc.sh \
	   file://display-box-ready.sh \
	   file://box-ready-msg.sh \
                  "

S = "${WORKDIR}"

do_install() {
             install -d ${D}${bindir}
	     install -d ${D}${sysconfdir}/network/if-pre-up.d
	     install -d ${D}${sysconfdir}/network/if-up.d
	     install -d ${D}${sysconfdir}/udhcpc.d
	     install -d ${D}${sysconfdir}/init.d
             install -d ${D}${sysconfdir}/rc5.d
             
	     install -m 0755 displog.sh ${D}${bindir}
             install -m 0755 if-pre-up.sh ${D}${sysconfdir}/network/if-pre-up.d/display
             install -m 0755 if-up.sh ${D}${sysconfdir}/network/if-up.d/display
             install -m 0755 udhcpc.sh ${D}${sysconfdir}/udhcpc.d/99display

	     install -m 0755 ${S}/box-ready-msg.sh ${D}${bindir}/box-ready-msg
             install -m 0755 ${S}/display-box-ready.sh ${D}${sysconfdir}/init.d
             ln -s ${sysconfdir}/init.d/display-box-ready.sh ${D}${sysconfdir}/rc5.d/S80display-box-ready
}

