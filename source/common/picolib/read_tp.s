// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright © 2020 Keith Packard

/*
 * This cannot be a C ABI function as the compiler assumes that it
 * does not modify anything other than r0 and lr.
 */
	.syntax unified
	.text
	.align 4
	.p2align 4,,15
	.global __aeabi_read_tp
	.type __aeabi_read_tp,%function
#ifdef __thumb__
	.thumb
#endif

__aeabi_read_tp:
	.cfi_sections .debug_frame
	.cfi_startproc
	/* Load the address of __tls */
	ldr r0,=__tls
	/* Dereference to get the value of __tls */
	ldr r0,[r0]
	/* All done, return to caller */
	bx lr
	.cfi_endproc
