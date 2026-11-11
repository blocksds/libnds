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


// This function switches to abort mode to do the context change because it
// isn't possible to set CPSR, PC and update the Thumb/ARM execution mode with a
// single instruction. Modifying CPSR by hand has undefined results when
// changing the Thumb flag, for example. The only safe way to do it is to switch
// to a privileged mode with its own spsr (like abort) and switch from there.
// The abort handler of the BIOS sets the stack pointer as soon as it starts, so
// this doesn't break the abort handler either.
BEGIN_ASM_FUNC cothread_context_switch_to_active itcm

    // Save scheduler context

    push    {r4-r11, lr}
    ldr     r0, =cothread_scheduler_sp
    str     sp, [r0]

    // Load active thread context

    ldr     r0, =cothread_list_active
    ldr     r0, [r0] // The CPU state is right at the start of the context

    ldr     sp, [r0, #COTHREAD_INFO_CPU_SP_OFFSET]
    ldr     lr, [r0, #COTHREAD_INFO_CPU_LR_OFFSET]

    msr     cpsr, #(CPSR_MODE_ABORT | CPSR_FLAG_IRQ_DIS) // Keep IRQs off

    ldr     r1, [r0, #COTHREAD_INFO_CPU_CPSR_OFFSET]
    msr     spsr, r1

    ldr     lr, [r0, #COTHREAD_INFO_CPU_PC_OFFSET]

    mov     sp, r0
    ldmia   sp, {r0-r12}

    movs    r15, r14 // Return from abort mode


// We don't need to save r0-r3 from this function. The thread is yielding
// manually, which means it has used a function call, and the compiler only
// expects registers {r4-r11, lr} to be preserved.
BEGIN_ASM_FUNC cothread_context_switch_to_scheduler itcm

    // Save active context

    ldr     r0, =cothread_list_active
    ldr     r0, [r0] // The CPU state is right at the start of the context

    add     r0, #COTHREAD_INFO_CPU_R4_OFFSET // Skip r0-r3
    stmia   r0!, {r4-r12, sp, lr} // r12 isn't needed, but it's faster to save it

    // When saving CPSR we need to check LR to see if the return function was in
    // ARM or Thumb mode so that we can set the Thumb flag in CPSR as well.
    mrs     r1, cpsr
    tst     lr, #1
    orrne   r1, #CPSR_FLAG_THUMB
    str     r1, [r0, #4] // Save cpsr

    str     lr, [r0] // Save pc

    // Disable interrupts before entering the scheduler

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
