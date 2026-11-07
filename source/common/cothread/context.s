// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors

#include <nds/asminc.h>

    .syntax unified

    .arm


BEGIN_ASM_FUNC cothread_context_resume

    // Save scheduler context
    push    {r4-r11, lr}
    ldr     r0, =cothread_scheduler_sp
    str     sp, [r0]

    // Load active thread context
    ldr     r0, =cothread_active_thread
    ldr     r0, [r0]
    ldr     sp, [r0, #0]
    pop     {r4-r11, lr}

    bx      lr


BEGIN_ASM_FUNC cothread_context_yield

    // Save active context
    push    {r4-r11, lr}
    ldr     r0, =cothread_active_thread
    ldr     r0, [r0]
    str     sp, [r0, #0]

    // Load scheduler context
    ldr     r0, =cothread_scheduler_sp
    ldr     sp, [r0]
    pop     {r4-r11, lr}

    bx      lr


BEGIN_ASM_FUNC cothread_context_entrypoint

    // r4 = arg
    // r5 = user entrypoint

    mov     r0, r4 // Load argument
#ifdef ARM9
    blx     r5
#else
    mov     lr, pc // Load return address (current + 8)
    bx      r5     // Jump to entrypoint
#endif

    // r0 = return value

    ldr     r1, =cothread_active_thread
    ldr     r1, [r1]
    str     r0, [r1, #4] // Save return value in 'arg'

    // Set "joined" flag. The thread can't resume anymore.
    mov     r2, #0x80000000
    str     r2, [r1, #0]

    // Load scheduler context
    ldr     r1, =cothread_scheduler_sp
    ldr     sp, [r1]
    pop     {r4-r11, lr}

    bx      lr
