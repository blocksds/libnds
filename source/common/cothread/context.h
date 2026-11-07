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
void cothread_context_entrypoint(cothread_info_t *ctx);

/// Starts/resumes a given coroutine.
void cothread_context_resume(cothread_info_t* ctx);

/// Yields a given value of a coroutine back to the caller.
void cothread_context_yield(cothread_info_t* ctx);

#endif // LIBNDS_CONTEXT_H__
