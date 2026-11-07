// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef LIBNDS_CONTEXT_H__
#define LIBNDS_CONTEXT_H__

#include <stddef.h>
#include <stdint.h>

#include <nds/cothread.h>

// Initializes the context of a new thread.
void cothread_context_initialize(cothread_info_t *ctx, void *sp_top,
                                 int (*user_entrypoint)(void*), void *arg);

// System entrypoint for threads. It loads the argument stored in the context
// and jumps to the user entrypoint.
void cothread_context_entrypoint(void);

// Yield from the scheduler and start/resume the active thread.
void cothread_context_resume(void);

// Yields from the current thread and returns to the scheduler.
void cothread_context_yield(void);

#endif // LIBNDS_CONTEXT_H__
