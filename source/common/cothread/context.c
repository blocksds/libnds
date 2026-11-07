// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors

#include "common/cothread/context.h"

void cothread_ndsabi_coro_make_noctx(cothread_info_t* __restrict__ coro,
                              void* __restrict__ sp_top,
                              int(*coproc)(void*),
                              void *arg)
{
    void cothread_ndsabi_coro_pop_noctx(void);
    uintptr_t pop_function = (uintptr_t)&cothread_ndsabi_coro_pop_noctx;

    /* Allocate space for storing r4-r11, lr, self, and entry procedure (11 words) */
    unsigned int *stack = sp_top;
    stack -= 11;
    stack[8] = (unsigned int) pop_function; /* lr */
    stack[9] = (unsigned int) coro;
    stack[10] = (unsigned int) coproc;

    coro->arm_sp = ((unsigned int) stack) & 0x7fffffff;
    coro->joined = 0; /* Clear joined flag (ready to start) */
    coro->arg = (unsigned int)arg;
}
