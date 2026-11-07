// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Adrian "asie" Siekierka
// Copyright (C) 2026 Antonio Niño Díaz

#include <nds/cothread.h>
#include <nds/exceptions.h>
#include <nds/memory.h>

#pragma GCC optimize("-Os")

// dlmalloc configuration

#ifndef NDEBUG
# define FOOTERS 1
# define INSECURE 0
#else
# define FOOTERS 0
# define INSECURE 1
#endif

#ifndef NDEBUG
static void corruption_error_action(void)
{
    libndsCrash("dlmalloc corruption error");
}

static void usage_error_action(void)
{
    libndsCrash("dlmalloc user error");
}

#define CORRUPTION_ERROR_ACTION(mstate)         corruption_error_action()
#define USAGE_ERROR_ACTION(mstate, chunk_ptr)   usage_error_action()
#endif // NDEBUG

#define MSPACES 1
#define ONLY_MSPACES 0

// This macro is a bit hacky, it relies on getting private variable 'm' from the
// function that uses the macro.
#define MORECORE(size) libnds_multi_heap_sbrk(is_global(m), size)
static void *libnds_multi_heap_sbrk(bool is_global, size_t size);
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
#define DESTROY_LOCK(lk) (0)
#define TRY_LOCK(lk) (comutex_try_acquire((lk)) ? 0 : 1)

// malloc implementations and aliases

#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Walloc-size"
#pragma GCC diagnostic ignored "-Wunused-function"

#include "dlmalloc_impl.h"

// Create aliases for some functions present in picolibc but not in dlmalloc to
// prevent the linker from including any malloc code from picolibc.

void *aligned_alloc(size_t, size_t) __attribute__((alias("memalign"), leaf, nothrow));
void *__malloc_malloc(size_t) __attribute__((alias("malloc"), leaf, malloc, nothrow, __alloc_size__(1)));
void __malloc_free(void *) __attribute__((alias("free"), leaf, nothrow));
void cfree(void *) __attribute__((alias("free"), leaf, nothrow));

// Custom functions to use dlmalloc in DTCM and ITCM
// =================================================

// Linker symbols
extern char __sbss_end[];      // End of statically-allocated data in reserved space in DTCM
extern char __dtcm_data_top[]; // End of total reserved space by the user in DTCM

extern char __dtcm_data_size[]; // Total size of the reserved space by the user

extern char __itcm_end[];   // End of statically-allocated data and code in ITCM

// Only try to initialize the allocators once, even if they have failed. They
// depend on statically-allocated variables and code, which doesn't change while
// the application is running.
static bool itcm_dlmalloc_initialized = false;
static bool dtcm_dlmalloc_initialized = false;

// Context of the allocators
static mspace itcm_mspace = 0;
static mspace dtcm_mspace = 0;

// dlmalloc uses less than `128 * sizeof(size_t))` bytes (512 bytes). Check how
// much free ITCM/DTCM is available and initialize the allocators only if it
// makes sense.
const size_t minimum_available_space = 1024;

static void itcm_dlmalloc_init(void)
{
    if (itcm_dlmalloc_initialized)
        return;

    const uintptr_t ITCM_HEAP_START = (uintptr_t)__itcm_end;
    const size_t ITCM_HEAP_SIZE = ITCM_END - ITCM_HEAP_START;

    if (ITCM_HEAP_SIZE >= minimum_available_space)
    {
        // Require a system lock
        itcm_mspace = create_mspace_with_base((void *)ITCM_HEAP_START, ITCM_HEAP_SIZE, 1);
    }

    itcm_dlmalloc_initialized = true;
}

static void dtcm_dlmalloc_init(void)
{
    if (dtcm_dlmalloc_initialized)
        return;

    // Make sure that there's enough space to use the allocator.
    const uintptr_t DTCM_HEAP_START = (uintptr_t)__sbss_end;
    const uintptr_t DTCM_HEAP_END = (uintptr_t)__dtcm_data_top;
    const size_t DTCM_HEAP_SIZE = DTCM_HEAP_END - DTCM_HEAP_START;

    if (DTCM_HEAP_SIZE > minimum_available_space)
    {
        // Require a system lock
        dtcm_mspace = create_mspace_with_base((void *)DTCM_HEAP_START, DTCM_HEAP_SIZE, 1);
    }

    dtcm_dlmalloc_initialized = true;
}

static void *libnds_multi_heap_sbrk(bool is_global, size_t size)
{
    // This function is called by dlmalloc functions whenever a mspace runs out
    // of memory: they call sys_alloc(), which calls CALL_MORECORE() (which
    // expands to MORECORE(), defined in this file).
    //
    // If the user tries to allocate memory in ITCM/DTCM, but there is no more
    // space for it, we don't want the allocation to return space in main RAM,
    // so this function detects which mspace is being used, and it only calls
    // sbrk() when allocating from main RAM.
    if (!is_global)
        return NULL;

    return sbrk(size);
}

// ITCM

void *itcm_malloc(size_t size)
{
    itcm_dlmalloc_init();

    if (itcm_mspace == 0)
        return NULL;

    return mspace_malloc(itcm_mspace, size);
}

void *itcm_calloc(size_t n, size_t size)
{
    itcm_dlmalloc_init();

    if (itcm_mspace == 0)
        return NULL;

    return mspace_calloc(itcm_mspace, n, size);
}

void *itcm_realloc(void *p, size_t size)
{
    itcm_dlmalloc_init();

    if (itcm_mspace == 0)
        return NULL;

    return mspace_realloc(itcm_mspace, p, size);
}

void *itcm_memalign(size_t alignment, size_t size)
{
    itcm_dlmalloc_init();

    if (itcm_mspace == 0)
        return NULL;

    return mspace_memalign(itcm_mspace, alignment, size);
}

void *itcm_aligned_alloc(size_t alignment, size_t size)
{
    return itcm_memalign(alignment, size);
}

void itcm_free(void *ptr)
{
    if (itcm_mspace == 0)
        return;

    mspace_free(itcm_mspace, ptr);
}

// DTCM

void *dtcm_malloc(size_t size)
{
    dtcm_dlmalloc_init();

    if (dtcm_mspace == 0)
        return NULL;

    return mspace_malloc(dtcm_mspace, size);
}

void *dtcm_calloc(size_t n, size_t size)
{
    dtcm_dlmalloc_init();

    if (dtcm_mspace == 0)
        return NULL;

    return mspace_calloc(dtcm_mspace, n, size);
}

void *dtcm_realloc(void *p, size_t size)
{
    dtcm_dlmalloc_init();

    if (dtcm_mspace == 0)
        return NULL;

    return mspace_realloc(dtcm_mspace, p, size);
}

void *dtcm_memalign(size_t alignment, size_t size)
{
    dtcm_dlmalloc_init();

    if (dtcm_mspace == 0)
        return NULL;

    return mspace_memalign(dtcm_mspace, alignment, size);
}

void *dtcm_aligned_alloc(size_t alignment, size_t size)
{
    return dtcm_memalign(alignment, size);
}

void dtcm_free(void *ptr)
{
    if (dtcm_mspace == 0)
        return;

    mspace_free(dtcm_mspace, ptr);
}
