// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/asminc.h>

    .syntax  unified

    .arm

    .extern irqTable

#ifdef ARM9
BEGIN_ASM_FUNC IntrMain itcm
#endif
#ifdef ARM7
BEGIN_ASM_FUNC IntrMain
#endif

    mov     r12, #0x4000000         // REG_BASE

    ldr     r1, [r12, #0x208]       // r1 = IME
    cmp     r1, #0
    bxeq    lr

    mov     r0, #0
    str     r0, [r12, #0x208]       // disable IME
    mrs     r0, spsr
    stmfd   sp!, {r0-r1, r12, lr}   // {spsr, IME, REG_BASE, lr_irq}

    add     r12, r12, #0x210
    ldmia   r12, {r1, r2}
    ands    r1, r1, r2
    ldr     r2, =irqTable

    // Notify the BIOS
    ldr     r0, =__irq_flags        // defined by linker script
    ldr     r3, [r0]
    orr     r3, r3, r1
    str     r3, [r0]

    // Set the interruption bits to notify the cothread scheduler to run any
    // yielded thread that was waiting for them.
    ldr     r0, =cothread_irq_flags
    ldr     r3, [r0]
    orr     r3, r3, r1
    str     r3, [r0]
#ifdef ARM7
    bne     endsetflags

    add     r12, r12, #8
    ldmia   r12, {r1, r2}
    ands    r1, r1, r2
    ldr     r2, =irqTableAUX

    // Notify the BIOS
    ldr     r0, =__irq_flagsaux
    ldr     r3, [r0]
    orr     r3, r3, r1
    str     r3, [r0]

    // Set the interruption bits to notify the cothread scheduler to run any
    // yielded thread that was waiting for them.
    ldr     r0, =cothread_irq_aux_flags
    ldr     r3, [r0]
    orr     r3, r3, r1
    str     r3, [r0]
#endif
endsetflags:

findIRQ:

    ldr     r0, [r2, #4]
    cmp     r0, #0
    beq     no_handler
    ands    r0, r0, r1
    bne     jump_intr
    add     r2, r2, #8
    b       findIRQ

no_handler:

    str     r1, [r12, #4]           // IF Clear
    ldmfd   sp!, {r0-r1, r12, lr}   // {spsr, IME, REG_BASE, lr_irq}
    str     r1, [r12, #0x208]       // restore REG_IME
    mov     pc, lr

jump_intr:

    ldr     r1, [r2]                // user IRQ handler address
    cmp     r1, #0
    bne     got_handler
    mov     r1, r0
    b       no_handler

got_handler:

    str     r0, [r12, #4]           // IF Clear

    mrs     r2, cpsr
    mov     r3, r2
    bic     r3, r3, #0xdf           // Enable IRQ & FIQ. Set CPU mode to System
    orr     r3, r3, #0x1f
    msr     cpsr,r3

    push    {r2, lr}
    adr     lr, IntrRet
    bx      r1

IntrRet:

    mov     r12, #0x4000000         // REG_BASE
    str     r12, [r12, #0x208]      // disable IME
    pop     {r2, lr}

    msr     cpsr, r2

    ldmfd   sp!, {r0-r1, r12, lr}   // {spsr, IME, REG_BASE, lr_irq}
    msr     spsr, r0                // Restore SPSR
    str     r1, [r12, #0x208]       // Restore REG_IME
    mov     pc, lr

    .pool
    .end
