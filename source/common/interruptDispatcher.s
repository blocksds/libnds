// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2024 Adrian "asie" Siekierka
// Copyright (C) 2025 Antonio Niño Díaz

#include <nds/asminc.h>
#include <nds/cothread_asm.h>
#include <nds/cpu_asm.h>

// TODO: Enable this code in the ARM7 if threads are enabled in the ARM7
#ifdef ARM9
#define THREADS_ENABLED
#endif

    .syntax  unified

    .arm

// Find the highest set IRQ bit
//
// Input:
//     r1 = interrupt mask
// Output:
//     r0 = Position of the highest bit set in r1
.macro do_ctz
#ifdef ARM9
    clz     r0, r1      // This counts leading zeroes, not trailing zeroes
    rsb     r0, r0, #31 // Flip to convert to trailing zeroes: r0 = 31 - r0
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
    // If it's equal to 2 or 3, add 1 to r0
    add     r0, r0, r1, lsr #1
#endif
.endm

    .set REG_BASE,      0x04000000
    .set OFFSET_IME,    0x208
    .set OFFSET_IE,     0x210
    .set OFFSET_IF,     0x214
    .set OFFSET_AUXIE,  0x218
    .set OFFSET_AUXIF,  0x21C

// Global interrupt dispatcher of libnds
#ifdef ARM9
BEGIN_ASM_FUNC IntrMain itcm
#endif
#ifdef ARM7
BEGIN_ASM_FUNC IntrMain
#endif
    // Note: The BIOS saves the following registers, so we are free to use them
    // here without saving them before: {r0-r3, r12, lr}

    mov     r12, #REG_BASE

    // We can assume that if we're here:
    // - ARM9 and ARM7: IME = 1
    // - ARM9:          IE & IF != 0
    // - ARM7:          (IE & IF != 0) || (AUXIE & AUXIF != 0)

    ldr     r1, [r12, #OFFSET_IE]
    ldr     r2, [r12, #OFFSET_IF]
    ands    r1, r1, r2              // r1 = IE & IF

#ifdef ARM9
    moveq   pc, lr                  // if (IE & IF == 0) return
#endif
#ifdef ARM7
    // Check if no main IRQ bits are set. This can only happen in DSi mode, so
    // there is no need to check the __dsimode flag. If no bits are set, we need
    // to check AUX IRQs.
    beq     check_aux_irqs
#endif

    do_ctz

    mov     r1, #1
    mov     r1, r1, lsl r0

    // r0 = Index of IRQ with the highest priority
    // r1 = Bit of IRQ with the highest priority

    str     r1, [r12, #OFFSET_IF]   // Clear that bit in IF

    // Notify the BIOS
    ldr     r2, =__irq_flags        // Symbol defined by linker script
    ldr     r3, [r2]
    orr     r3, r3, r1
    str     r3, [r2]

#ifdef THREADS_ENABLED
    // Get pointer to array of pointers of cothread threads waiting for IRQs.
    ldr     r2, =cothread_list_irq
    add     r2, r2, r0, lsl #2
#endif

    // Calculate address of the IRQ vector
    ldr     r3, =irqTable
    add     r3, r3, r0, lsl #2

    // r2 = Pointer to list of threads waiting for this interrupt
    // r3 = Target IRQ table address

#ifdef ARM7
    b       main_irq_found

check_aux_irqs:

    ldr     r1, [r12, #OFFSET_AUXIE]
    ldr     r2, [r12, #OFFSET_AUXIF]
    ands    r1, r1, r2              // r1 = AUXIE & AUXIF

    moveq   pc, lr                  // if (AUXIE & AUXIF == 0) return

    do_ctz

    mov     r1, #1
    mov     r1, r1, lsl r0

    // r0 = Index of IRQ with the highest priority
    // r1 = Bit of IRQ with the highest priority

    str     r1, [r12, #OFFSET_AUXIF] // Clear that bit in AUXIF

    // Notify the BIOS
    ldr     r2, =__irq_flagsaux     // Symbol defined by linker script
    ldr     r3, [r2]
    orr     r3, r3, r1
    str     r3, [r2]

#ifdef THREADS_ENABLED
    // Get pointer to array of pointers of cothread threads waiting for IRQs.
    ldr     r2, =cothread_list_irq_aux
    add     r2, r2, r0, lsl #2
#endif

    // Calculate address of the IRQ vector
    ldr     r3, =irqTableAUX
    add     r3, r3, r0, lsl #2

    // r2 = Pointer to list of threads waiting for this interrupt
    // r3 = Target IRQ table address

main_irq_found:

#endif

    // Check if there is no user IRQ handler
    ldr     r3, [r3]
    cmp     r3, #0
    beq     clear_threads_and_exit

    // r2 = Pointer to list of threads waiting for this interrupt
    // r3 = Address of user IRQ handler

    // If we've reached this point, we have found an interrupt handler, we have
    // cleared the IF/AUXIF bit we're going to handle, we've notified the BIOS,
    // and we've notified the cothread library.

    ldr     r1, [r12, #OFFSET_IME]  // r1 = IME

    // Disable IME because we're going to enable interrupts in CPSR but we want
    // to let the user decide whether interrupts can be nested or not.
    str     r12, [r12, #OFFSET_IME] // Only bit 0 is used, it's okay to write
                                    // any value with bit 0 clear.

    mrs     r0, spsr
#ifdef THREADS_ENABLED
    push    {r0-r2, r12, lr}        // {spsr_irq, IME, thread list, REG_BASE, lr_irq}
#else
    push    {r0-r1, r12, lr}        // {spsr_irq, IME, REG_BASE, lr_irq}
#endif

    // Increment counter of nested interrupts being handled
    adr     r0, irq_nesting_level
    ldrh    r1, [r0]
    add     r1, r1, #1
    strh    r1, [r0]

    // Enable IRQ and FIQ, set mode to System
    mrs     r0, cpsr
    bic     r1, r0, #(CPSR_FLAG_IRQ_DIS | CPSR_FLAG_FIQ_DIS | CPSR_MODE_MASK)
    orr     r1, r1, #CPSR_MODE_SYSTEM
    msr     cpsr, r1

    push    {r0, lr}                // {old cpsr, lr_sys}

#ifdef ARM9
    blx     r3
#endif

#ifdef ARM7
    adr     lr, interrupt_return
    bx      r3
interrupt_return:
#endif

    pop     {r0, lr}                // {old cpsr, lr_sys}

    msr     cpsr, r0                // Return to IRQ mode with IRQs and FIQs disabled

#ifdef THREADS_ENABLED
    pop     {r0-r2, r12, lr}        // {spsr_irq, IME, thread list, REG_BASE, lr_irq}
#else
    pop     {r0-r1, r12, lr}        // {spsr_irq, IME, REG_BASE, lr_irq}
#endif
    msr     spsr, r0                // Restore SPSR
    str     r1, [r12, #OFFSET_IME]  // Restore REG_IME

    // Decrement counter of nested interrupts being handled
    adr     r0, irq_nesting_level
    ldrh    r1, [r0]
    sub     r1, r1, #1
    strh    r1, [r0]

    // After handling all high priority things, unblock any threads that were
    // waiting for this interrupt.

clear_threads_and_exit:

#ifdef THREADS_ENABLED

    // r2 = Pointer to list of threads waiting for this interrupt

    mov     r1, #0 // Counter of how many threads are resumed
    mov     r12, #0 // This will be used to clear pointers
clear_next_thread:
    ldr     r0, [r2]
    cmp     r0, #0
    beq     exit // Exit if we have reached the end of the list

    str     r12, [r2] // Clear this pointer

    // Clear the "waiting for IRQ" flag
    ldr     r3, [r0, #COTHREAD_INFO_FLAGS_OFFSET]
    bic     r3, r3, #COTHREAD_WAITING
    str     r3, [r0, #COTHREAD_INFO_FLAGS_OFFSET]

    add     r2, r0, COTHREAD_INFO_NEXT_IRQ_OFFSET

    add     r1, r1, #1

    b       clear_next_thread

exit:

    // If we haven't resumed any thread, there's nothing to do
    cmp     r1, #0
    moveq   pc, lr

    // Decrease number of threads waiting for interrupts
    ldr     r0, =cothread_threads_waiting_count
    ldr     r2, [r0]
    sub     r2, r2, r1
    str     r2, [r0]

#endif // THREADS_ENABLED

    mov     pc, lr

    .pool

// This variable holds the current number of user interrupt handlers that have
// been nested. If 0, no interrupt is currently being handled.
    .global irq_nesting_level
irq_nesting_level:
    .space  2

    .end
