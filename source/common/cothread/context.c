// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
// Copyright (C) 2026 Antonio Niño Díaz

#include "common/cothread/context.h"

void cothread_context_initialize(cothread_info_t *ctx, void *sp_top,
                                 int (*user_entrypoint)(void*), void *arg)
{
    // Allocate space for storing r4-r11, lr, context, and user entrypoint
    unsigned int *stack = sp_top;
    stack -= 11; // 11 words

    // stack[0...7] = r4...r11 // We don't need to initialize them
    stack[8] = (uintptr_t)&cothread_context_entrypoint; // lr
    stack[9] = (uintptr_t)ctx;
    stack[10] = (uintptr_t)user_entrypoint;

    ctx->arm_sp = (uintptr_t)stack;
    ctx->joined = 0; // Clear joined flag (ready to start)
    ctx->arg = (uintptr_t)arg;
}
