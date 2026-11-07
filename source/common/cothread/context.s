// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors

#include <nds/asminc.h>

    .syntax unified

    .arm


BEGIN_ASM_FUNC cothread_ndsabi_coro_resume

    push    {r4-r11, lr}
    mov     r1, sp

    ldr     sp, [r0]
    pop     {r4-r11, lr}
    str     r1, [r0]

    bx      lr


BEGIN_ASM_FUNC cothread_ndsabi_coro_yield

    push    {r4-r11, lr}
    mov     r1, sp

    ldr     sp, [r0]
    pop     {r4-r11, lr}
    str     r1, [r0]

    bx      lr


BEGIN_ASM_FUNC cothread_ndsabi_coro_pop_noctx

    ldr     r0, [r0, #4] // Load argument

    ldr     r2, [sp, #4] // Load entrypoint
#ifdef ARM9
    blx     r2
#else
    mov     lr, pc       // Load return address (current + 8)
    bx      r2           // Jump to entrypoint
#endif

    ldr     r1, [sp]
    // r0 contains return value
    // r1 points to cothread_info_t*

    // Load suspended context
    ldr     sp, [r1]
    pop     {r4-r11, lr}

    // Set "joined" flag. The thread can't resume anymore.
    mov     r2, #0x80000000
    str     r2, [r1]

    bx      lr
