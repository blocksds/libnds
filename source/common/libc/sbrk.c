// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2025 Antonio Niño Díaz

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

void *fake_heap_end = NULL;
void *fake_heap_start = NULL;

// Next address to be used. It is updated after every call to sbrk()
static uintptr_t heap_start = 0;

void *sbrk(int incr)
{
    // Get heap start
    if (heap_start == 0)
    {
        assert(fake_heap_start != NULL); // Fake heap not initialized
        heap_start = (uintptr_t)fake_heap_start;
    }

    assert(fake_heap_end != NULL); // Fake heap not initialized

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
