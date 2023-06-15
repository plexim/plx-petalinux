

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

# Overwrite settings from petalinux-config to use tag on branch other than master
SRC_URI =+ "git://github.com/enclustra-bsp/xilinx-uboot;nobranch=1"
SRC_URI_remove="git://github.com/enclustra-bsp/xilinx-uboot;branch=master"

SRC_URI += "file://platform-top.h"
SRC_URI += "file://bsp.cfg"
SRC_URI += "file://0001-Added-configuration-for-RT-Box-2.patch"

