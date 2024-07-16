// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2023 Gericom

#include <nds/asminc.h>

    .syntax unified
#ifdef ARM9
    // It is unsafe to start the DMA from main RAM in the ARM9
    .section ".itcm", "ax"
#elif defined(ARM7)
    .text
#else
#error "ARM9 or ARM7 must be defined."
#endif
    .arm

    // r0 = DMA number
    // r1 = src
    // r2 = dst
    // r3 = control
BEGIN_ASM_FUNC dmaSetParams

    ldr     r12, =0x040000B0
    add     r0, r0, r0, lsl #1 // r0 = r0 * 3
    add     r12, r12, r0, lsl #2
    stmia   r12, {r1, r2, r3}
    b       1f // Delay for safety
1:
    bx      lr


    // r0 = DMA number
BEGIN_ASM_FUNC dmaStopSafe

    // Disable IRQs
    mrs     r3, cpsr
    orr     r1, r3, #0x80
    msr     cpsr_c, r1

    ldr     r12, =0x040000BA
    add     r0, r0, r0, lsl #1 // r0 = r0 * 3
    add     r12, r12, r0, lsl #2
    ldrh    r0, [r12]
    bic     r0, r0, #0x3A00 // Clear mode and repeat bits
    strh    r0, [r12]
    // Delay for safety
    b       1f
1:
#ifdef ARM9
    b       2f
2:
#endif
    ldrh    r0, [r12]
    bic     r0, r0, #0x8000 // Clear enable bit
    strh    r0, [r12]

    // Restore IRQs
    msr     cpsr_c, r3

    bx      lr

    .pool
    .end
