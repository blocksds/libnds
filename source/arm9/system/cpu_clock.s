// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 fincs

#include <nds/asminc.h>

    .syntax  unified
    .arch    armv5te
    .cpu     arm946e-s

    .arm

BEGIN_ASM_FUNC setCpuClock itcm
    mov     r12, #0x4000000
    ldrb    r3, [r12, #0x208]   // Preserve REG_IME in r3
    strb    r12, [r12, #0x208]  // Disable interrupts (REG_IME = 0)
    push    {r3}

    cmp     r0, #0
    ldr     r3, =0x4004004
    ldrh    r0, [r3]            // Read REG_SCFG_CLK
    mov     r2, #8              // CPU clock cycles to wait
    biceq   r1, r0, #1          // Adjust bit 0 according to requested clock
    orrne   r1, r0, #1
    cmp     r1, r0
    and     r0, r0, #1          // Set return value (old CPU clock)
    beq     restoreIrq          // Skip switch if clock already in desired state

    strh    r1, [r3]            // Update REG_SCFG_CLK
1:  subs    r2, r2, #1          // Wait for CPU clock to stabilize
    bge     1b

restoreIrq:
    pop     {r3}
    strb    r3, [r12, #0x208]   // Restore the previous REG_IME

    bx      lr
