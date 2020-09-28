/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


	.globl	spin_lock
	.globl	spin_unlock

        /*
         * This macro is used to mark the end of a function.
         */
        .macro endfunc _name
        .cfi_endproc
        .size \_name, . - \_name
        .endm

        .macro func _name, _align=2
        /*
         * Add Call Frame Information entry in the .debug_frame section for
         * debugger consumption. This enables callstack printing in debuggers.
         * This does not use any space in the final loaded binary, only in the
         * ELF file.
         * Note that a function manipulating the CFA pointer location (i.e. the
         * x29 frame pointer on AArch64) should declare it using the
         * appropriate .cfi* directives, or be prepared to have a degraded
         * debugging experience.
         */
        .cfi_sections .debug_frame
        .section .text.asm.\_name, "ax"
        .type \_name, %function
        /*
         * .cfi_startproc and .cfi_endproc are needed to output entries in
         * .debug_frame
         */
        .cfi_startproc
        .align \_align
        \_name:
#if ENABLE_BTI
        /* When Branch Target Identification is enabled, insert "bti jc"
         * instruction to enable indirect calls and branches
         */
         bti    jc
#endif
        .endm

/*
 * Acquire lock using load-/store-exclusive instruction pair.
 *
 * void spin_lock(spinlock_t *lock);
 */
func spin_lock
	mov	w2, #1
	sevl
l1:	wfe
l2:	ldaxr	w1, [x0]
	cbnz	w1, l1
	stxr	w1, w2, [x0]
	cbnz	w1, l2
	ret
endfunc spin_lock

/*
 * Release lock previously acquired by spin_lock.
 *
 * Use store-release to unconditionally clear the spinlock variable.
 * Store operation generates an event to all cores waiting in WFE
 * when address is monitored by the global monitor.
 *
 * void spin_unlock(spinlock_t *lock);
 */
func spin_unlock
	stlr	wzr, [x0]
	ret
endfunc spin_unlock
