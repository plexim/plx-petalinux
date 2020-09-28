#
# This is the config-eeprom apllication recipe
#
#

SUMMARY = "eeprom_config application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = " \
file://AtmelAtsha204a.c \
file://AtmelAtsha204a.h \
file://AtmelAtsha204aTypes.h \
file://CurrentMonitor.c \
file://CurrentMonitor.h \
file://ErrorCodes.h \
file://GlobalTypes.h \
file://GlobalVariables.h \
file://I2cExample.c \
file://I2cExampleDefines.h \
file://I2cInterface.c \
file://I2cInterface.h \
file://Makefile \
file://ModuleConfigConstants.c \
file://ModuleConfigConstants.h \
file://ModuleConfigValueKeys.c \
file://ModuleEeprom.c \
file://ModuleEeprom.h \
file://ReadSystemMonitor.c \
file://ReadSystemMonitor.h \
file://RealtimeClock.c \
file://RealtimeClock.h \
file://StandardIncludes.h \
file://SystemController.c \
file://SystemController.h \
file://SystemDefinitions.h \
file://SystemMonitor.c \
file://SystemMonitor.h \
file://TargetEnvironment.h \
file://TargetModuleConfig.h \
file://UtilityFunctions.h \
          "

S = "${WORKDIR}"

do_compile() {
        oe_runmake
}

do_install() {
        install -d ${D}${bindir}
        install -m 0755 ${S}/eeprom_config ${D}${bindir}
}

