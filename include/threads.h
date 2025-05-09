// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka

#ifndef THREADS_H__
#define THREADS_H__

#include <nds/cothread.h>

// Partial implementation of C11 threads.h.

#ifdef __cplusplus
extern "C" {
#endif

typedef cothread_t thrd_t;
enum {
    thrd_success = 0,
    thrd_error = -1,
    thrd_timedout = -2,
    thrd_busy = -3,
    thrd_nomem = -4
};

typedef void (*thrd_start_t)(void*);

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg);
int thrd_join(thrd_t thr, int *res);

static inline thrd_t thrd_current(void)
{
    return cothread_get_current();
}

static inline int thrd_detach(thrd_t thr)
{
    return cothread_detach(thr);
}

static inline int thrd_equal(thrd_t thr0, thrd_t thr1)
{
    return thr0 == thr1;
}

static inline void thrd_yield(void)
{
    cothread_yield();
}

typedef comutex_t mtx_t;
enum
{
    mtx_plain = 0
};

static inline int mtx_init(mtx_t *mtx, int type)
{
    (void)type;
    return comutex_init(mtx) ? thrd_success : thrd_error;
}

static inline int mtx_lock(mtx_t *mtx)
{
    comutex_acquire(mtx);
    return thrd_success;
}

static inline int mtx_trylock(mtx_t *mtx)
{
    return comutex_try_acquire(mtx) ? thrd_success : thrd_error;
}

static inline int mtx_unlock(mtx_t *mtx)
{
    comutex_release(mtx);
    return thrd_success;
}

#ifdef __cplusplus
}
#endif

#endif // THREADS_H__
