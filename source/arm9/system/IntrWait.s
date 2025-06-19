// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2016 Dave Murphy (WinterMute)
// Copyright (C) 2025 Antonio Niño Díaz

#include <nds/asminc.h>
#include <nds/arm9/cp15_asm.h>

    .syntax  unified
    .arch    armv5te
    .cpu     arm946e-s

    .arm

BEGIN_ASM_FUNC swiWaitForVBlank

    mov     r0, #1
    mov     r1, #1

    // Fallthrough

BEGIN_ASM_FUNC_NO_SECTION swiIntrWait

    // Disable interrupts (REG_IME = 0)
    mov     r12, #0x4000000
    str     r12, [r12, #0x208]

    // Prepare a pointer to the BIOS IRQ flags
    ldr     r3, =__irq_flags

    // Check mode
    cmp     r0, #0
    beq     mode_return_if_flags_set

    // r0 = 1 : Discard old flags, wait until a new flag becomes set

mode_discard_old_flags:

    // Discard old flags
    ldr     r2, [r3]
    bic     r2, r2, r1
    str     r2, [r3]

    b       wait_loop

mode_return_if_flags_set:

    // r0 = 0 : Return immediately if an old flag was already set

    // Check flags and clear them if they are set
    ldr     r2, [r3]
    ands    r0, r1, r2
    eorne   r2, r2, r0
    strne   r2, [r3]
    // Exit if there was a flag already set
    bne     exit

    // Wait loop. Common for both modes
wait_loop:

    // Enable interrupts
    mov     r2, #1
    str     r2, [r12, #0x208]

    mov     r0, #0
    mcr     CP15_REG7_WAIT_FOR_INTERRUPT

    // Disable interrupts
    str     r0, [r12, #0x208]

    // Check flags and clear them if they are set
    ldr     r2, [r3]
    ands    r0, r1, r2
    eorne   r2, r2, r0
    strne   r2, [r3]

    beq     wait_loop

exit:
    // Enable interrupts
    mov     r12, #0x4000000
    mov     r2, #1
    str     r2, [r12, #0x208]

    bx      lr
