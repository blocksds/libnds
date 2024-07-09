// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2006-2016 Dave Murphy (WinterMute)

#include <nds/asminc.h>

    .syntax  unified
    .cpu     arm7tdmi

    .arm

BEGIN_ASM_FUNC swiWaitForVBlank

    mov     r0, #1
    mov     r1, #1
    mov     r2, #0
    nop

BEGIN_ASM_FUNC swiIntrWait

    stmfd   sp!, {lr}
    cmp     r0, #0
    blne    testirq

wait_irq:
    swi     #(6 << 16) // swiHalt
    bl      testirq
    beq     wait_irq
    ldmfd   sp!, {lr}
    bx      lr

testirq:
    // REG_IME = 0
    mov     r12, #0x4000000
    strb    r12, [r12, #0x208]

    // Acknowledge interrupt in BIOS flags register
    ldr     r3, [r12, #-8]
    ands    r0, r1, r3
    eorne   r3, r3, r0
    strne   r3, [r12, #-8]

    // REG_IME = 1
    mov     r0, #1
    strb    r0, [r12, #0x208]

    bx      lr
