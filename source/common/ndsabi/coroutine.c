// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
//
// Support:
//    __ndsabi_coro_make

#include <ndsabi.h>

static void __ndsabi_coro_make_internal(
                    __ndsabi_coro_t* __restrict__ coro,
                    void* __restrict__ sp_top,
                    int(*coproc)(__ndsabi_coro_t*, void*),
                    unsigned int pop_function, void *arg)
{
    /* AAPCS wants stack to be aligned to 8 bytes */
    unsigned int alignedTop = ((unsigned int) sp_top) & ~0x7u;
    unsigned int* stack = (unsigned int*) alignedTop;

    /* Allocate space for storing r4-r11, lr, self, and entry procedure (11 words) */
    stack -= 11;
    stack[8] = (unsigned int) pop_function; /* lr */
    stack[9] = (unsigned int) coro;
    stack[10] = (unsigned int) coproc;

    coro->arm_sp = ((unsigned int) stack) & 0x7fffffff;
    coro->joined = 0; /* Clear joined flag (ready to start) */
    coro->arg = (unsigned int)arg;
}

void __ndsabi_coro_make(__ndsabi_coro_t* __restrict__ coro,
                        void* __restrict__ sp_top,
                        int(*coproc)(__ndsabi_coro_t*, void*),
                        void *arg)
{
    void __ndsabi_coro_pop(void);

    __ndsabi_coro_make_internal(coro, sp_top, coproc,
                                (unsigned int)__ndsabi_coro_pop, arg);
}

void __ndsabi_coro_make_noctx(__ndsabi_coro_t* __restrict__ coro,
                              void* __restrict__ sp_top,
                              int(*coproc)(void*),
                              void *arg)
{
    void __ndsabi_coro_pop_noctx(void);
    typedef int (*coproc_ctx_fn)(__ndsabi_coro_t*, void*);

    __ndsabi_coro_make_internal(coro, sp_top, (coproc_ctx_fn)(uintptr_t)coproc,
                                (unsigned int)__ndsabi_coro_pop_noctx, arg);
}
