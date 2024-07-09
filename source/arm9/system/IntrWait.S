// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2016 Dave Murphy (WinterMute)

#include <nds/asminc.h>
#include <nds/arm9/cp15_asm.h>

    .syntax  unified
    .arch    armv5te
    .cpu     arm946e-s

    .arm

BEGIN_ASM_FUNC swiWaitForVBlank

    mov     r0, #1
    mov     r1, #1

BEGIN_ASM_FUNC swiIntrWait

    push    {lr}

    // Disable interrupts (REG_IME = 0)
    mov     r12, #0x4000000
    str     r12, [r12, #0x208]

    // Get the DTCM base address
    mrc     CP15_REG9_DTCM_CONTROL(r3)
    mov     r3, r3, lsr #12 // Clear the lower 12 bits
    mov     r3, r3, lsl #12
    // Calculate the DTCM end address
    add     r3, r3, #0x4000

    mov     r12, r0
    bl      check_flags
    beq     wait

    cmp     r12, #0
    beq     flag_set
wait:
    mov     r12, #0x4000000
wait_flags:
    // Enable interrupts
    mov     r2, #1
    str     r2, [r12, #0x208]

    mov     r0, #0
    mcr     CP15_REG7_WAIT_FOR_INTERRUPT

    // Disable interrupts
    str     r12, [r12, #0x208]
    bl      check_flags
    beq     wait_flags

flag_set:
    // Enable interrupts
    mov     r2, #1
    str     r2, [r12, #0x208]
    pop     {pc}

check_flags:
    // The BIOS flags are stored 8 bytes before the end of DTCM
    ldr     r2, [r3, #-8]
    ands    r0, r1, r2
    eorne   r2, r2, r0
    strne   r2, [r3, #-8]
    bx      lr
