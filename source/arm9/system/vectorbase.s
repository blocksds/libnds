// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2009 Dave Murphy (WinterMute)

#include <nds/arm9/cp15_asm.h>
#include <nds/asminc.h>

    .syntax  unified
    .arch    armv5te
    .cpu     arm946e-s

    .arm

    .section    .vectors,"ax",%progbits

    .global     SystemVectors

    ldr     r15, vec_reset
    ldr     r15, vec_undefined
    ldr     r15, vec_swi
    ldr     r15, vec_prefetch_abort
    ldr     r15, vec_data_abort
dummy:      b   dummy
    ldr     r15, vec_irq
    ldr     r15, vec_fiq

SystemVectors:
vec_reset:
    .word   0xFFFF0000
vec_undefined:
    .word   0xFFFF0004
vec_swi:
    .word   0xFFFF0008
vec_prefetch_abort:
    .word   0xFFFF000C
vec_data_abort:
    .word   0xFFFF0010
vec_irq:
    .word   0xFFFF0018
vec_fiq:
    .word   0xFFFF001C

BEGIN_ASM_FUNC setVectorBase vectors

    // Load the CP15 Control Register
    mrc     CP15_REG1_CONTROL_REGISTER(r1)

    // if (highVector)
    //     r1 |= CP15_CONTROL_ALTERNATE_VECTOR_SELECT
    // else
    //     r1 &= ~CP15_CONTROL_ALTERNATE_VECTOR_SELECT

    cmp     r0, #0
    biceq   r1, r1, #CP15_CONTROL_ALTERNATE_VECTOR_SELECT
    orrne   r1, r1, #CP15_CONTROL_ALTERNATE_VECTOR_SELECT

    // Store the control register
    mcr     CP15_REG1_CONTROL_REGISTER(r1)

    bx      lr
