// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef LIBNDS_MALLOC_H__
#define LIBNDS_MALLOC_H__

#if defined(ARM7)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include_next <malloc.h>
#pragma GCC diagnostic pop

#elif defined(ARM9)

// Don't redefine the functions that are already in stdlib.h
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mallinfo
{
    size_t arena;    // non-mmapped space allocated from system
    size_t ordblks;  // number of free chunks
    size_t smblks;   // always 0
    size_t hblks;    // always 0
    size_t hblkhd;   // space in mmapped regions
    size_t usmblks;  // maximum total allocated space
    size_t fsmblks;  // always 0
    size_t uordblks; // total allocated space
    size_t fordblks; // total free space
    size_t keepcost; // releasable (via malloc_trim) space
};

void *memalign(size_t alignment, size_t size) __malloc_like __warn_unused_result
__alloc_size(2) __nothrow;

struct mallinfo mallinfo(void);
void malloc_stats(void);
int mallopt(int param_number, int value);
size_t malloc_usable_size(void *p);
int malloc_trim(size_t pad);

// mallopt options

#define M_TRIM_THRESHOLD     (-1)
#define M_GRANULARITY        (-2)
#define M_MMAP_THRESHOLD     (-3)

#ifdef __cplusplus
}
#endif

#endif // ARM9

#endif // LIBNDS_MALLOC_H__
