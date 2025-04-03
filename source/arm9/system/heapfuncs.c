// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 Dave Murphy (WinterMute)
// Copyright (C) 2025 Antonio Niño Díaz

#include <unistd.h>

#include <nds/ndstypes.h>
#include <nds/system.h>

extern void *fake_heap_end;   // Current heap start
extern void *fake_heap_start; // Current heap end

u8 *getHeapStart(void)
{
    return fake_heap_start;
}

u8 *getHeapEnd(void)
{
    return (u8 *)sbrk(0);
}

u8 *getHeapLimit(void)
{
    return fake_heap_end;
}

int reduceHeapSize(size_t size_to_save)
{
    // Check that the size is a multiple of 4
    if ((size_to_save & 3) != 0)
        return -1;

    u8 *new_end = getHeapLimit() - size_to_save;

    // Check that the heap hasn't already passed the new end
    if (new_end < getHeapEnd())
        return -2;

    fake_heap_end = new_end;

    return 0;
}
