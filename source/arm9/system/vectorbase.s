// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2009 Dave Murphy (WinterMute)

#include <nds/asminc.h>

	.section	.vectors,"ax",%progbits

	.global		SystemVectors

	.arm

	ldr	r15,vec_reset
	ldr	r15,vec_undefined
	ldr	r15,vec_swi
	ldr	r15,vec_prefetch_abort
	ldr	r15,vec_data_abort
dummy:	b	dummy
	ldr	r15,vec_irq
	ldr	r15,vec_fiq

SystemVectors:
vec_reset:
	.word	0xFFFF0000
vec_undefined:
	.word	0xFFFF0004
vec_swi:
	.word	0xFFFF0008
vec_prefetch_abort:
	.word	0xFFFF000C
vec_data_abort:
	.word	0xFFFF0010
vec_irq:
	.word	0xFFFF0018
vec_fiq:
	.word	0xFFFF001C


@---------------------------------------------------------------------------------
BEGIN_ASM_FUNC setVectorBase vectors
@---------------------------------------------------------------------------------
 
	@ load the CP15 Control Register
	mrc p15, 0, r1, c1, c0, 0
 
	@ if (highVector)
	@   // set the bit
	@ r1 |= (1<<13)
	@ else
	@   // clear the bit
	@ r1 &= ~(1<<13)
 
	cmp r0, #0
	biceq r1, r1, #(1<<13)
	orrne r1, r1, #(1<<13)
 
	@ store the control register
	mcr p15, 0, r1, c1, c0, 0
 
	bx lr

