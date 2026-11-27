// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#include <nds/asminc.h>
#include <nds/cpu_asm.h>

    .syntax unified
    .arm


.macro COMUTEX_TO_SIGNAL_ID out_signal, in_mutex
    orr     \out_signal, \in_mutex, #(1 << 31)
.endm


BEGIN_ASM_FUNC comutex_init // (comutex_t *mutex)

    // *mutex = 0
    mov     r1, #0
    strb    r1, [r0]

    // Return true
    mov     r0, #1
    bx      lr


BEGIN_ASM_FUNC comutex_try_acquire // (comutex_t *mutex)

    // Mark the mutex as acquired and fetch its previous state
    mov     r1, #1
    swpb    r2, r1, [r0] // r2 = [r0] ; [r0] = r1

    // Return 0 (failed to acquire) if the previous value was 1 (acquired)
    // Return 1 (could acquire it) if the previous value was 0 (free)
    eor     r0, r2, #1

    bx      lr


BEGIN_ASM_FUNC comutex_acquire // (comutex_t *mutex)

    push    {r4, lr}

    // Preserve the mutex pointer in a callee-saved register
    mov     r4, r0

loop:
    // Disable interrupts
    mrs     r0, cpsr
    orr     r0, r0, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r0

    // Mark the mutex as acquired and fetch its previous state
    mov     r0, #1
    swpb    r1, r0, [r4] // r1 = [r4] ; [r4] = r0

    // If the previous value was 0 (free), we've managed to acquire it
    cmp     r1, #0
    beq     done

    // Get signal ID for this mutex
    COMUTEX_TO_SIGNAL_ID r0, r4

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

done:
    // Enable interrupts
    mrs     r0, cpsr
    bic     r0, r0, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r0

    pop     {r4, lr}

    bx      lr


BEGIN_ASM_FUNC comutex_release // (comutex_t *mutex)

    // *mutex = 0
    mov     r1, #0
    strb    r1, [r0]

    // Get signal ID for this mutex
    COMUTEX_TO_SIGNAL_ID r0, r0

    // Tail call
    ldr     r1, =cothread_send_signal
    bx      r1
