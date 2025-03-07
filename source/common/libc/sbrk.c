// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023-2025 Antonio Niño Díaz

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#ifdef ARM9
#include <nds/arm9/sassert.h>
#endif

void *fake_heap_end = NULL;
void *fake_heap_start = NULL;

// Next address to be used. It is updated after every call to sbrk()
static uintptr_t heap_start = 0;

void *sbrk(int incr)
{
    // Get heap start
    if (heap_start == 0)
    {
#ifdef ARM9
        sassert(fake_heap_start != NULL, "Fake heap not initialized");
#endif
        heap_start = (uintptr_t)fake_heap_start;
    }

#ifdef ARM9
    sassert(fake_heap_end != NULL, "Fake heap not initialized");
#endif

    // Get heap end
    uintptr_t heap_end = (uintptr_t)fake_heap_end;

    // Try to allocate
    if (heap_start + incr > heap_end)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    uintptr_t prev_heap_start = heap_start;

    heap_start += incr;

    return (void *)prev_heap_start;
}
