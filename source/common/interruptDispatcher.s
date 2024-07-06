// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2024 Adrian "asie" Siekierka

#include <nds/asminc.h>
#include <nds/cpu_asm.h>

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

    str     r12, [r12, #0x208]      // disable IME
                                    // (as only bit 0 is used, it's okay to write
                                    // any value with bit 0 clear)
    mrs     r0, spsr
    stmfd   sp!, {r0-r1, r12, lr}   // {spsr, IME, REG_BASE, lr_irq}

    add     r12, r12, #0x210
    ldmia   r12!, {r1, r2}          // read IE, IF
    ands    r1, r1, r2
#ifdef ARM9
    beq     no_handler
#else
    beq     setflagsaux
#endif
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
    b       findIRQ

setflagsaux:
    // Since the interrupt wasn't a main one, it must be an AUX one.
    // This way, we don't have to check if we're in DSi mode here.

    ldmia   r12!, {r1, r2}           // read AUX IE, IF
    ands    r1, r1, r2
    beq     no_handler
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

    // find the highest set IRQ bit and adjust the mask/table address
    // accordingly
    //
    // input:
    // r1 = interrupt mask
    // r2 = irq table address
    //
    // output:
    // r1 = target interrupt mask
    // r2 = target irq table address
    //
    // r0, r3 can be used freely
findIRQ:
    // set r0 to the position of the highest bit set in r1
#ifdef ARM9
    clz     r0, r1
    eor     r0, r0, #31
#else
    mov     r0, #0
    cmp     r1, #0x10000
    movcs   r1, r1, lsr #16
    addcs   r0, r0, #16
    tst     r1, #0xFF00
    movne   r1, r1, lsr #8
    addne   r0, r0, #8
    tst     r1, #0xF0
    movne   r1, r1, lsr #4
    addne   r0, r0, #4
    tst     r1, #0xC
    movne   r1, r1, lsr #2
    addne   r0, r0, #2
    // r1 is now equal to 0, 1, 2 or 3
    // if it's equal to 2 or 3, add 1 to r0
    add     r0, r0, r1, lsr #1
#endif
    // adjust r1 and r2 based on the bit index in r0
    mov     r1, #1
    mov     r1, r1, lsl r0
    add     r2, r2, r0, lsl #2

    // clear the IRQ now being serviced
    str     r1, [r12, #-4]

    // compare dummy IRQ address with found IRQ address
    // this skips some setup required for jumping to an IRQ handler
    ldr     r0, =irqDummy
    ldr     r1, [r2]
    cmp     r1, r0
    beq     no_handler

got_handler:

    // enable IRQ and FIQ, set mode to System
    mrs     r2, cpsr
    bic     r3, r2, #(CPSR_FLAG_IRQ_DIS | CPSR_FLAG_FIQ_DIS | CPSR_MODE_MASK)
    orr     r3, r3, #CPSR_MODE_SYSTEM
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
                                    // (as only bit 0 is used, it's okay to write
                                    // any value with bit 0 clear)
    pop     {r2, lr}

    msr     cpsr, r2

no_handler:

    ldmfd   sp!, {r0-r1, r12, lr}   // {spsr, IME, REG_BASE, lr_irq}
    msr     spsr, r0                // Restore SPSR
    str     r1, [r12, #0x208]       // Restore REG_IME
    mov     pc, lr

    .pool
    .end
