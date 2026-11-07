// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
// Copyright (C) 2026 Antonio Niño Díaz

#include "common/cothread/context.h"

void cothread_context_initialize(cothread_info_t *ctx, void *sp_top,
                                 int (*user_entrypoint)(void*), void *arg)
{
    // This thread will run for the first time after a cothread_context_resume()
    // call from the scheduler. It will jump to the address in LR (so we need to
    // load the system thread entrypoint there) and load all registers that we
    // have prepared here.

    // Allocate space for storing r4-r11, lr
    uint32_t *stack = sp_top;
    stack -= 10; // 9 words (+ 1 padding word for alignment to 64 bit)

    stack[0] = (uintptr_t)arg; // r4
    stack[1] = (uintptr_t)user_entrypoint; // r5
    //stack[2...7] = r6...r11 // We don't need to initialize them
    stack[8] = (uintptr_t)&cothread_context_entrypoint; // lr = actual entrypoint

    ctx->arm_sp = (uintptr_t)stack;
    ctx->joined = 0; // Clear joined flag (ready to start)
}
