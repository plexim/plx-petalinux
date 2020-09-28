FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://0001-SDEI-Pop-dispatch-context-only-after-error-checking.patch \
    file://0001-SDEI-Ensure-SDEI-handler-executes-with-CVE-2018-3639.patch \
    file://0001-SDEI-Allow-platforms-to-define-explicit-events.patch \
    file://0001-SDEI-Determine-client-EL-from-NS-context-s-SCR_EL3.patch \
    file://0001-SDEI-Make-dispatches-synchronous.patch \
    file://0001-SDEI-Fix-dispatch-bug.patch \
    file://0001-SDEI-Fix-determining-client-EL.patch \
    file://0001-SDEI-Fix-name-of-internal-function.patch \
    file://0001-SDEI-Fix-locking-issues.patch \
    file://0001-SDEI-Mask-events-after-CPU-wakeup.patch \
    file://0001-sdei-include-context.h-to-fix-compilation-errors.patch \
    file://0001-SDEI-Unconditionally-resume-Secure-if-it-was-interru.patch \
    file://0001-BL31-Introduce-jump-primitives.patch \
    file://0001-Make-setjmp.h-prototypes-comply-with-the-C-standard.patch \
    file://0001-zynqmp-add-sdei-support.patch \
    file://0001-GIC-Fix-setting-interrupt-configuration.patch \
    file://0001-gicv2-Fix-support-for-systems-without-secure-interru.patch \
    file://0001-plat-xilinx-zynqmp-Use-GIC-framework-for-warm-restar.patch \
    "

# OUTPUT_DIR = "${B}/${PLATFORM}/debug"

