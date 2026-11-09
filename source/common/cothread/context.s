// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
// Copyright (C) 2026 Antonio Niño Díaz

#include <nds/asminc.h>
#include <nds/cothread_asm.h>

    .syntax unified
    .arm


BEGIN_ASM_FUNC cothread_context_switch_to_active itcm

    // Save scheduler context
    push    {r4-r11, lr}
    ldr     r0, =cothread_scheduler_sp
    str     sp, [r0]

    // Load active thread context
    ldr     r0, =cothread_list_active
    ldr     r0, [r0]
    ldr     sp, [r0, #COTHREAD_INFO_STACK_POINTER_OFFSET]
    pop     {r4-r11, lr}

    bx      lr


BEGIN_ASM_FUNC cothread_context_switch_to_scheduler itcm

    // Save active context
    push    {r4-r11, lr}
    ldr     r0, =cothread_list_active
    ldr     r0, [r0]
    str     sp, [r0, #COTHREAD_INFO_STACK_POINTER_OFFSET]

    // Load scheduler context
    ldr     r0, =cothread_scheduler_sp
    ldr     sp, [r0]
    pop     {r4-r11, lr}

    bx      lr


BEGIN_ASM_FUNC cothread_context_entrypoint itcm

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

    ldr     r1, =cothread_list_active
    ldr     r1, [r1]

    // Save return value in 'exit_code'
    str     r0, [r1, #COTHREAD_INFO_EXIT_CODE_OFFSET]

    // Mark the thread as ended
    mov     r0, #COTHREAD_STATE_ENDED
    strb    r0, [r1, #COTHREAD_INFO_STATE_OFFSET]

    // Load scheduler context
    ldr     r0, =cothread_scheduler_sp
    ldr     sp, [r0]
    pop     {r4-r11, lr}

    bx      lr
