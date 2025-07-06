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

    // Fallthrough

BEGIN_ASM_FUNC_NO_SECTION swiIntrWait

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
    ldr     r3, [r12, #-8] // 0x0380FFF8
    ands    r0, r1, r3
    eorne   r3, r3, r0
    strne   r3, [r12, #-8]

    // REG_IME = 1
    mov     r0, #1
    strb    r0, [r12, #0x208]

    bx      lr

BEGIN_ASM_FUNC_NO_SECTION swiIntrWaitAUX

    stmfd   sp!, {lr}
    cmp     r0, #0
    blne    testirq_aux

wait_irq_aux:
    swi     #(6 << 16) // swiHalt
    bl      testirq_aux
    beq     wait_irq_aux
    ldmfd   sp!, {lr}
    bx      lr

testirq_aux:
    // REG_IME = 0
    mov     r12, #0x4000000
    strb    r12, [r12, #0x208]

    // Acknowledge interrupt in BIOS flags register
    ldr     r3, [r12, #-8] // 0x0380FFF8
    ands    r0, r1, r3
    eorne   r3, r3, r0
    strne   r3, [r12, #-8]

    // Acknowledge interrupt in BIOS flags register
    ldr     r3, [r12, #-0x40] // 0x0380FFC0
    ands    r0, r2, r3
    eorne   r3, r3, r0
    strne   r3, [r12, #-0x40]

    // REG_IME = 1
    mov     r0, #1
    strb    r0, [r12, #0x208]

    bx      lr
