// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
// Copyright (C) 2026 Antonio Niño Díaz

#include <nds/cpu_asm.h>
#include <nds/cothread.h>

#include "common/cothread/context.h"

// This is the context of the scheduler when it gets saved to the stack. It
// doesn't need to save r0-r3, r12 and cpsr because it can't be interrupted, it
// can only be exited manually through a function call, and it only expects
// registers r4-r11 and lr to be preserved.
//
// typedef struct
// {
//     uint32_t r4, r5, r6, r7, r8, r9, r10, r11;
//     uint32_t lr;
// }
// cothread_sheduler_saved_context_t;

void cothread_context_initialize(cothread_info_t *ctx, void *sp_top,
                                 int (*user_entrypoint)(void*), void *arg)
{
    ctx->cpu.r0 = (uintptr_t)arg;
    ctx->cpu.r5 = (uintptr_t)user_entrypoint;
    ctx->cpu.sp = (uintptr_t)sp_top;
    ctx->cpu.pc = (uintptr_t)&cothread_context_entrypoint; // System entrypoint
    ctx->cpu.cpsr = CPSR_MODE_SYSTEM; // ARM mode. Interrupts enabled by default
}
