// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/arm9/cp15_asm.h>
#include <nds/asminc.h>

    .syntax  unified
    .arch    armv5te
    .cpu     arm946e-s

    .arm

BEGIN_ASM_FUNC enterException

    // Store context
    ldr     r12, =exceptionRegisters
    stmia   r12, {r0-r11}
    str     r13, [r12, #(oldStack - exceptionRegisters)]

    // Assign a stack
    ldr     r13, =exceptionStack
    ldr     r13, [r13]

    // Re-enable MPU
    mrc     CP15_REG1_CONTROL_REGISTER(r0)
    orr     r0, r0, #CP15_CONTROL_PROTECTION_UNIT_ENABLE
    mcr     CP15_REG1_CONTROL_REGISTER(r0)

    // BIOS exception stack
    ldr     r0, =0x02FFFD90

    // Get r15 from BIOS exception stack
    ldr     r2, [r0, #8]
    str     r2, [r12, #(reg15 - exceptionRegisters)]

    // Get stored r12 and SPSR from bios exception stack
    ldmia   r0, {r2,r12}

    // Get banked registers from correct processor mode
    mrs     r3, cpsr
    bic     r4, r3, #0x1F
    and     r2, r2, #0x1F

    // Check for user mode and use system mode instead
    cmp     r2, #0x10
    moveq   r2, #0x1F

    orr     r4, r4, r2
    msr     cpsr, r4
    ldr     r0, =reg12
    stmia   r0, {r12-r14}
    msr     cpsr, r3

    // Get C function and call it
    ldr     r12, =exceptionC
    ldr     r12, [r12, #0]
    blxne   r12

    // Restore registers
    ldr     r12, =exceptionRegisters
    ldmia   r12, {r0-r11}
    ldr     r13, [r12, #(oldStack - exceptionRegisters)]

    // Return through BIOS
    mov    pc, lr


    .global exceptionC
exceptionC:
    .word    0x00000000

    .global exceptionStack
exceptionStack:
    .word    0x02ff4000

    .global exceptionRegisters
exceptionRegisters:
    .space  12 * 4 // r0 to r11
reg12:      .word   0
reg13:      .word   0
reg14:      .word   0
reg15:      .word   0
oldStack:   .word   0
