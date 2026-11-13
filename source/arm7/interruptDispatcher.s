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
    .set OFFSET_AUXIE,  0x218
    .set OFFSET_AUXIF,  0x21C

// Find the highest set IRQ bit
//
// Input:
//     r0 = interrupt mask
// Output:
//     r1 = Position of the highest bit set in r0
.macro do_ctz
    mov     r1, #0
    cmp     r0, #0x10000
    movcs   r0, r0, lsr #16
    addcs   r1, r1, #16
    tst     r0, #0xFF00
    movne   r0, r0, lsr #8
    addne   r1, r1, #8
    tst     r0, #0xF0
    movne   r0, r0, lsr #4
    addne   r1, r1, #4
    tst     r0, #0xC
    movne   r0, r0, lsr #2
    addne   r1, r1, #2
    // r0 is now equal to 0, 1, 2 or 3
    // If it's equal to 2 or 3, add 1 to r1
    add     r1, r1, r0, lsr #1
.endm

BEGIN_ASM_FUNC IntrMain

    // Note: The BIOS saves the following registers, so we are free to use them
    // in the handler without saving them before: {r0-r3, r12, lr}

    mov     r12, #REG_BASE

    // We can assume that if we're here:
    // - IME = 1
    // - (IE & IF != 0) || (AUXIE & AUXIF != 0)

    ldr     r0, [r12, #OFFSET_IE]
    ldr     r2, [r12, #OFFSET_IF]
    ands    r0, r0, r2              // IE & IF

    // Check if no main IRQ bits are set. This can only happen in DSi mode, so
    // there is no need to check the __dsimode flag. If no bits are set, we need
    // to check AUX IRQs.
    beq     check_aux_irqs

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

    ldr     r1, =cothread_send_irq

    // r0 = Mask of the IRQ with the highest priority
    // r1 = Address of cothread check function
    // r3 = Target IRQ table address

    b       main_irq_found

check_aux_irqs:

    ldr     r0, [r12, #OFFSET_AUXIE]
    ldr     r2, [r12, #OFFSET_AUXIF]
    ands    r0, r0, r2              // AUXIE & AUXIF

    moveq   pc, lr                  // if (AUXIE & AUXIF == 0) return

    do_ctz

    mov     r0, #1
    mov     r0, r0, lsl r1

    // r0 = Mask of the IRQ with the highest priority
    // r1 = Index of IRQ with the highest priority

    str     r0, [r12, #OFFSET_AUXIF] // Clear that bit in AUXIF

    // Notify the BIOS
    ldr     r2, =__irq_flagsaux     // Symbol defined by linker script
    ldr     r3, [r2]
    orr     r3, r3, r0
    str     r3, [r2]

    // Calculate address of the IRQ vector
    ldr     r3, =irqTableAUX
    add     r3, r3, r1, lsl #2

    ldr     r1, =cothread_send_irq_aux

    // r0 = Mask of the IRQ with the highest priority
    // r1 = Address of cothread check function
    // r3 = Target IRQ table address

main_irq_found:

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

    push    {r0-r2, r12, lr} // {IRQ bit, cothread function, IME, spsr_irq, lr_irq}

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
    adr     lr, interrupt_return
    bx      r3
interrupt_return:
    pop     {lr} // {lr_sys}

    // Return to IRQ mode with IRQs and FIQs disabled
    msr     cpsr, #(CPSR_MODE_IRQ | CPSR_FLAG_IRQ_DIS | CPSR_FLAG_FIQ_DIS)

    pop     {r0-r2, r12, lr} // {IRQ bit, cothread function, IME, spsr_irq, lr_irq}

    msr     spsr, r12               // Restore SPSR

    mov     r12, #REG_BASE
    str     r2, [r12, #OFFSET_IME]  // Restore REG_IME

    // Decrement counter of nested interrupts being handled
    adr     r12, irq_nesting_level
    ldrh    r3, [r12]
    sub     r3, r3, #1
    strh    r3, [r12]

clear_threads_and_exit:

    // After handling all high priority things, unblock any threads that were
    // waiting for this interrupt.

    // r0 = Mask of the IRQ with the highest priority
    // r1 = Address of cothread check function

    // This will preserve r4-r11. We don't care about the other registers.
    push    {lr}
    adr     lr, check_return
    bx      r1   // Jump to the right cothread function to wake up threads
check_return:
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
