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

    // r1 = interrupt mask
    // r2 = irq table address
    // r0, r3 can be used freely

    // check if mask empty
    cmp     r1, #0
    streq   r1, [r12, #4]           // IF Clear
    beq     no_handler

    // find the highest set IRQ bit
findIRQ:
#ifdef ARM9
    clz     r0, r1
    eor     r0, r0, #31
    add     r2, r2, r0, lsl #2
#else
    ldr     r0, =#0xFFFF0000
    tst     r1, r0
    mov     r0, r1
    movne   r0, r0, lsr #16
    addne   r2, r2, #64
    tst     r0, #0xFF00
    movne   r0, r0, lsr #8
    addne   r2, r2, #32
    tst     r0, #0xF0
    movne   r0, r0, lsr #4
    addne   r2, r2, #16
    tst     r0, #0xC
    movne   r0, r0, lsr #2
    addne   r2, r2, #8
    tst     r0, #0x2
    movne   r0, r0, lsr #1
    addne   r2, r2, #4
#endif

    // compare dummy IRQ address with found IRQ address
    // this skips some setup required for jumping to an IRQ handler
    ldr     r0, =irqDummy
    str     r1, [r12, #4]           // IF Clear
    ldr     r1, [r2]
    cmp     r1, r0
    beq     no_handler

got_handler:

    mrs     r2, cpsr
    mov     r3, r2
    bic     r3, r3, #0xdf           // Enable IRQ & FIQ. Set CPU mode to System
    orr     r3, r3, #0x1f
    msr     cpsr, r3

    push    {r2, lr}
#ifdef ARM9
    blx     r1
#else
    adr     lr, IntrRet
    bx      r1
#endif

IntrRet:

    mov     r12, #0x4000000         // REG_BASE
    str     r12, [r12, #0x208]      // disable IME
    pop     {r2, lr}

    msr     cpsr, r2

no_handler:

    ldmfd   sp!, {r0-r1, r12, lr}   // {spsr, IME, REG_BASE, lr_irq}
    msr     spsr, r0                // Restore SPSR
    str     r1, [r12, #0x208]       // Restore REG_IME
    mov     pc, lr

    .pool
    .end
