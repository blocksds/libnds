// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Adrian "asie" Siekierka

#include <nds/cothread.h>

#pragma GCC optimize("-Os")

// dlmalloc configuration

#ifdef DEBUG
# define FOOTERS 1
# define INSECURE 0
#else
# define FOOTERS 0
# define INSECURE 1
#endif

#define HAVE_MORECORE 1
#define HAVE_MMAP 0
#define malloc_getpagesize 4096

// Global lock implementation

#define USE_LOCKS 2

static inline int ACQUIRE_LOCK(comutex_t *lk)
{
    comutex_acquire(lk);
    return 0;
}

static inline int RELEASE_LOCK(comutex_t *lk)
{
    comutex_release(lk);
    return 0;
}

#define MLOCK_T comutex_t
static comutex_t malloc_global_mutex;
#define INITIAL_LOCK(lk) comutex_init((lk))
#define DESTROY_LOCK(lk) {}
#define TRY_LOCK(lk) (comutex_try_acquire((lk))?0:1)

// malloc implementations and aliases

#undef DEBUG

#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Walloc-size"
#pragma GCC diagnostic ignored "-Wunused-function"

#include "dlmalloc_impl.h"

void *aligned_alloc(size_t, size_t) __attribute__((alias("memalign"), leaf, nothrow));
void *__malloc_malloc(size_t) __attribute__((alias("malloc"), leaf, malloc, nothrow, __alloc_size__(1)));
void __malloc_free(void *) __attribute__((alias("free"), leaf, nothrow));
void cfree(void *) __attribute__((alias("free"), leaf, nothrow));
