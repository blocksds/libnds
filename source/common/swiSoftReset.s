// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2009 Dave Murphy (WinterMute)

#include <nds/asminc.h>

	.text
	.align 4

	.arm
@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC swiSoftReset
@---------------------------------------------------------------------------------

    // Disable interrupts (REG_IME = 0)
    mov     r0, #0x4000000
    str     r0, [r0, #0x208]

#ifdef ARM7
	ldr	r0,=0x2FFFE34
#endif

#ifdef ARM9
	.arch	armv5te
	.cpu	arm946e-s
	ldr	r1, =0x00002078			@ disable TCM and protection unit
	mcr	p15, 0, r1, c1, c0
	@ Disable cache
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ Instruction cache
	mcr	p15, 0, r0, c7, c6, 0		@ Data cache

	@ Wait for write buffer to empty 
	mcr	p15, 0, r0, c7, c10, 4

	ldr	r0,=0x2FFFE24
#endif

	ldr	r0,[r0]
	bx	r0

	.pool
