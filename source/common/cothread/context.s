// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
// Copyright (C) 2026 Antonio Niño Díaz

#include <nds/asminc.h>
#include <nds/cpu_asm.h>
#include <nds/cothread_asm.h>

    .syntax unified
    .arm


BEGIN_ASM_FUNC cothread_context_switch_to_active itcm

    // Save scheduler context

    push    {r4-r11, lr}
    ldr     r0, =cothread_scheduler_sp
    str     sp, [r0]

    // Load active thread context

    ldr     lr, =cothread_list_active
    ldr     lr, [lr]

    ldr     r0, [lr, #(16 * 4)] // Load cpsr
    msr     cpsr, r0

    ldmia   lr!, {r0-r12, sp}
    ldr     lr, [lr, #0] // Load lr
    // Skip pc for now

    bx      lr


BEGIN_ASM_FUNC cothread_context_switch_to_scheduler itcm

    // Save active context

    ldr     r0, =cothread_list_active
    ldr     r0, [r0]

    add     r0, #(4 * 4) // We don't need to save r0-r3 from this function
    stmia   r0!, {r4-r12, sp, lr} // r12 isn't needed, but it's faster to save it
    str     lr, [r0], #4 // Save pc
    mrs     r1, cpsr
    str     r1, [r0] // Save cpsr

    // Disable interrupts

    msr     cpsr, #(CPSR_FLAG_IRQ_DIS | CPSR_MODE_SYSTEM)

    // Load scheduler context

    ldr     r0, =cothread_scheduler_sp
    ldr     sp, [r0]
    pop     {r4-r11, lr}

    bx      lr


BEGIN_ASM_FUNC cothread_context_entrypoint itcm

    // r0 = arg
    // r5 = user entrypoint

#ifdef ARM9
    blx     r5
#else
    mov     lr, pc // Load return address (current + 8)
    bx      r5     // Jump to entrypoint
#endif

    // r0 = return value

    // Get pointer to the current thread context
    ldr     r1, =cothread_list_active
    ldr     r1, [r1]

    // Save return value in 'exit_code'
    str     r0, [r1, #COTHREAD_INFO_EXIT_CODE_OFFSET]

    // Mark the thread as ended
    mov     r0, #COTHREAD_STATE_ENDED
    strb    r0, [r1, #COTHREAD_INFO_STATE_OFFSET]

    // Disable interrupts
    mrs     r0, cpsr
    orr     r0, r0, #CPSR_FLAG_IRQ_DIS
    msr     cpsr, r0

    // Load scheduler context
    ldr     r0, =cothread_scheduler_sp
    ldr     sp, [r0]
    pop     {r4-r11, lr}

    bx      lr
