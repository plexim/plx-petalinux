#
# This file is the eeprom-config recipe.
#

SUMMARY = "eeprom-config application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://AtmelAtsha204a.c \
	   file://AtmelAtsha204a.h \
	   file://AtmelAtsha204aTypes.h \
	   file://ErrorCodes.h \
	   file://GlobalTypes.h \
	   file://I2cInterface.c \
	   file://I2cInterface.h \
	   file://main.c \
	   file://Makefile \
	   file://ModuleConfigConstants.c \
	   file://ModuleConfigConstants.h \
	   file://ModuleConfigValueKeys.c \
	   file://ModuleEeprom.c \
	   file://ModuleEeprom.h \
	   file://StandardIncludes.h \
	   file://SystemDefinitions.h \
	   file://TargetModuleConfig.h \
	   file://UtilityFunctions.h \
		  "

S = "${WORKDIR}"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 eeprom_config ${D}${bindir}
}
