FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

# Overwrite settings from petalinux-config to use tag on branch other than master
SRC_URI =+ "git://github.com/enclustra-bsp/xilinx-uboot;nobranch=1"
SRC_URI_remove="git://github.com/enclustra-bsp/xilinx-uboot;branch=master"

SRC_URI += "file://platform-top.h \
            file://0001-Add-configuration-for-RT-Box-1.patch \
"
