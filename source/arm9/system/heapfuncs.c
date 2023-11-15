// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 Dave Murphy (WinterMute)

#include <unistd.h>

#include <nds/ndstypes.h>
#include <nds/system.h>

extern u8 *fake_heap_end;   // Current heap start
extern u8 *fake_heap_start; // Current heap end

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
