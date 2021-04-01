require jailhouse.inc

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}-git:"

SRC_URI = "git://github.com/siemens/jailhouse.git;branch=wip/arm64-zero-exits \
	  file://0001-core-Add-configuration-option-for-allowed-IDs-in-SMC.patch \
	  file://0003-hypervisor-Conditionally-forward-SiP-calls-to-ARM-tr.patch \
	  file://0004-no-kbuild-of-tools.patch \
	"

SRCREV = "ee0e5c42c9baf1389481c372693cd8fab0e1cb88"
PV = "0.12-git${SRCPV}"

CELLS = ""

DEFAULT_PREFERENCE = "-1"

