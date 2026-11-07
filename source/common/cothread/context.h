// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2021-2023 agbabi contributors

#ifndef LIBNDS_CONTEXT_H__
#define LIBNDS_CONTEXT_H__

#include <stddef.h>
#include <stdint.h>

#include <nds/cothread.h>

/// Initializes a coro struct to call a given coroutine.
///
/// The only difference is that the coroutine won't get the coroutine context as
/// an argument. Because of that, this function can't be used for simple
/// coroutines, it's designed to be used by a multithreading scheduler.
///
/// @param coro
///     Pointer to coro struct to initialize
/// @param sp_top
///     The TOP of the stack for this coroutine (stack grows down!)
/// @param coproc
///     Procedure to call as a coroutine
/// @param arg
///     Initial argument to be passed to the coroutine.
LIBNDS_NONNULL(1, 2, 3)
void cothread_ndsabi_coro_make_noctx(cothread_info_t* __restrict__ coro,
    void* __restrict__ sp_top, int(*coproc)(void*), void *arg);

/// Starts/resumes a given coroutine.
///
/// @param coro
///     Coroutine to start/resume
///
/// @return
///     Integer value from coroutine
LIBNDS_NONNULL(1)
int cothread_ndsabi_coro_resume(cothread_info_t* coro);

/// Yields a given value of a coroutine back to the caller.
///
/// @param coro
///     Coroutine that is yielding
LIBNDS_NONNULL(1)
void cothread_ndsabi_coro_yield(cothread_info_t* coro);

#endif // LIBNDS_CONTEXT_H__
