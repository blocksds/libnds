// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#include <nds/asminc.h>
#include <nds/cpu_asm.h>

    .syntax unified
    .arm


.macro COSEMA_TO_SIGNAL_ID out_signal, in_mutex
    orr     \out_signal, \in_mutex, #(1 << 31)
.endm

BEGIN_ASM_FUNC cosema_init // (cosema_t *sema, uint32_t init_val)

    // *sema = init_val
    str     r1, [r0]

    // Return true
    mov     r0, #1
    bx      lr


BEGIN_ASM_FUNC cosema_signal // (cosema_t *sema)

    // Disable interrupts
    mrs     r2, cpsr
    orr     r1, r2, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r1

    // *sema = *sema + 1
    ldr     r1, [r0]
    add     r1, r1, #1
    str     r1, [r0]

    // Enable interrupts
    msr     cpsr, r2

    // Get signal ID for this semaphore
    COSEMA_TO_SIGNAL_ID r0, r0

    // Tail call
    ldr     r1, =cothread_send_signal
    bx      r1


BEGIN_ASM_FUNC cosema_try_wait // (cosema_t *sema)

    // Disable interrupts
    mrs     r3, cpsr
    orr     r2, r3, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r2

    // *sema > 0 ?
    ldr     r1, [r0]
    cmp     r1, #0
    beq     try_failed

    // *sema = *sema - 1;
    sub     r1, r1, #1
    str     r1, [r0]

    // Enable interrupts
    msr     cpsr, r3

    // Return true
    mov     r0, #1
    bx      lr

try_failed:

    // (*sema == 0)

    // Enable interrupts
    msr     cpsr, r3

    // Return false
    mov     r0, #0
    bx      lr


BEGIN_ASM_FUNC cosema_wait // (cosema_t *sema)

    push    {r4, lr}

    // Preserve the semaphore pointer in a callee-saved register
    mov     r4, r0

loop:

    // Disable interrupts
    mrs     r3, cpsr
    orr     r3, r3, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r3

    // *sema > 0 ?
    ldr     r1, [r4]
    cmp     r1, #0
    bne     success

    // *sema == 0

    // Get signal ID for this semaphore
    COSEMA_TO_SIGNAL_ID r0, r4

    // Yield and wait for that signal
#ifdef ARM9
    blx     cothread_yield_signal
#else
    ldr     r1, =cothread_yield_signal
    mov     lr, yield_ret
    bx      r1
yield_ret:
#endif
    // IRQs are enabled after the thread resumes

    b       loop

success:

    // *sema > 0

    // *sema = *sema - 1;
    sub     r1, r1, #1
    str     r1, [r4]

    // Enable interrupts
    mrs     r3, cpsr
    bic     r3, r3, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r3

    pop     {r4, lr}

    bx      lr
