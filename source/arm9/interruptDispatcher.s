// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2024 Adrian "asie" Siekierka
// Copyright (C) 2025-2026 Antonio Niño Díaz

#include <nds/asminc.h>
#include <nds/cothread_asm.h>
#include <nds/cpu_asm.h>

    .syntax  unified

    .arm

    .set REG_BASE,      0x04000000
    .set OFFSET_IME,    0x208
    .set OFFSET_IE,     0x210
    .set OFFSET_IF,     0x214

// Find the highest set IRQ bit
//
// Input:
//     r0 = interrupt mask
// Output:
//     r1 = Position of the highest bit set in r0
.macro do_ctz
    clz     r1, r0      // This counts leading zeroes, not trailing zeroes
    rsb     r1, r1, #31 // Flip to convert to trailing zeroes: r1 = 31 - r1
.endm

// Global interrupt dispatcher of libnds
BEGIN_ASM_FUNC IntrMain itcm

    // Note: The BIOS saves the following registers, so we are free to use them
    // in the handler without saving them before: {r0-r3, r12, lr}

    mov     r12, #REG_BASE

    // We can assume that if we're here:
    // - IME = 1
    // - IE & IF != 0

    ldr     r0, [r12, #OFFSET_IE]
    ldr     r2, [r12, #OFFSET_IF]
    ands    r0, r0, r2              // IE & IF

    moveq   pc, lr                  // if (IE & IF == 0) return

    do_ctz

    mov     r0, #1
    mov     r0, r0, lsl r1

    // r0 = Mask of the IRQ with the highest priority
    // r1 = Index of IRQ with the highest priority

    str     r0, [r12, #OFFSET_IF]   // Clear that bit in IF

    // Notify the BIOS
    ldr     r2, =__irq_flags        // Symbol defined by linker script
    ldr     r3, [r2]
    orr     r3, r3, r0
    str     r3, [r2]

    // Calculate address of the IRQ vector
    ldr     r3, =irqTable
    add     r3, r3, r1, lsl #2

    // r0 = Mask of the IRQ with the highest priority
    // r3 = Target IRQ table address

    // Check if there is no user IRQ handler
    ldr     r3, [r3]
    cmp     r3, #0
    beq     clear_threads_and_exit

    // If we've reached this point, we have found an interrupt handler, we have
    // cleared the IF/AUXIF bit we're going to handle, we've notified the BIOS,
    // and we've notified the cothread library.

    ldr     r2, [r12, #OFFSET_IME]  // r2 = IME

    // Disable IME because we're going to enable interrupts in CPSR but we want
    // to let the user decide whether interrupts can be nested or not.
    str     r12, [r12, #OFFSET_IME] // Only bit 0 is used, it's okay to write
                                    // any value with bit 0 clear.

    mrs     r12, spsr

    push    {r0, r2, r12, lr} // {IRQ bit, IME, spsr_irq, lr_irq}

    // Increment counter of nested interrupts being handled
    adr     r0, irq_nesting_level
    ldrh    r1, [r0]
    add     r1, r1, #1
    strh    r1, [r0]

    // Enable IRQ and FIQ, set mode to System to use its stack. However, that
    // means we need to preserve its banked lr register.
    msr     cpsr, #CPSR_MODE_SYSTEM

    // Call user handler
    push    {lr} // {lr_sys}
    blx     r3
    pop     {lr} // {lr_sys}

    // Return to IRQ mode with IRQs and FIQs disabled
    msr     cpsr, #(CPSR_MODE_IRQ | CPSR_FLAG_IRQ_DIS | CPSR_FLAG_FIQ_DIS)

    pop     {r0, r2, r12, lr} // {IRQ bit, IME, spsr_irq, lr_irq}

    msr     spsr, r12

    mov     r12, #REG_BASE
    str     r2, [r12, #OFFSET_IME]  // Restore REG_IME

    // Decrement counter of nested interrupts being handled
    adr     r3, irq_nesting_level
    ldrh    r1, [r3]
    sub     r1, r1, #1
    strh    r1, [r3]

clear_threads_and_exit:

    // After handling all high priority things, unblock any threads that were
    // waiting for this interrupt.

    // r0 = Mask of the IRQ with the highest priority

    // This will preserve r4-r11. We don't care about the other registers.
    push    {lr}
    bl      cothread_send_irq
    pop     {lr}

    // Check if preemption is enabled. If so, save the current thread context
    // and switch to the scheduler.

    // Global pre-emption enable setting
    ldr     r0, =cothread_preemption_enabled
    ldrb    r0, [r0]
    cmp     r0, #0
    moveq   pc, lr

    // If we're in a nested interrupt, don't change thread context.
    adr     r0, irq_nesting_level
    ldrh    r1, [r0]
    cmp     r1, #0
    movne   pc, lr

    // Save context of the active thread

    ldr     r0, =cothread_list_active
    ldr     r0, [r0] // The CPU state is right at the start of the context

    add     r0, #COTHREAD_INFO_CPU_R4_OFFSET // Skip r0-r3 for now
    stmia   r0, {r4-r11}^ // Writeback not allowed in this mode
    add     r0, r0, #(COTHREAD_INFO_CPU_SP_OFFSET - COTHREAD_INFO_CPU_R4_OFFSET) // Skip r12 for now
    stmia   r0, {sp, lr}^ // Save user version of SP and LR

    mrs     r1, spsr
    str     r1, [r0, #(COTHREAD_INFO_CPU_CPSR_OFFSET - COTHREAD_INFO_CPU_SP_OFFSET)] // Save cpsr

    // We won't bother returning to the BIOS, so let's pop the registers saved
    // by the BIOS before calling the IRQ dispatcher. This will restore sp_irq
    // to the state it had before handling this IRQ, and we can do whatever we
    // want to exit this IRQ handler.
    pop     {r4-r7, r8, r9} // Load r0-r3, r12, lr

    sub     r0, r0, #COTHREAD_INFO_CPU_SP_OFFSET
    stmia   r0, {r4-r7} // Save r0-r3
    str     r8, [r0, #COTHREAD_INFO_CPU_R12_OFFSET] // Save r12
    sub     r9, r9, #4
    str     r9, [r0, #COTHREAD_INFO_CPU_PC_OFFSET] // Save lr in the pc

    // Switch to system mode manually to enter the scheduler without going
    // through the BIOS. Disable interrupts before entering it.

    msr     cpsr, #(CPSR_MODE_SYSTEM | CPSR_FLAG_IRQ_DIS)

    // Load scheduler context

    ldr     r0, =cothread_scheduler_sp
    ldr     sp, [r0]
    pop     {r4-r11, lr}

    bx      lr

    .pool

// This variable holds the current number of user interrupt handlers that have
// been nested. If 0, no interrupt is currently being handled.
    .global irq_nesting_level
irq_nesting_level:
    .space  2

    .end
