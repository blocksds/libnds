//////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2023 Antonio Niño Díaz
//
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any
//  damages arising from the use of this software.
//
//  Permission is granted to anyone to use this software for any
//  purpose, including commercial applications, and to alter it and
//  redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you
//     must not claim that you wrote the original software. If you use
//     this software in a product, an acknowledgment in the product
//     documentation would be appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and
//     must not be misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source
//     distribution.
//
//////////////////////////////////////////////////////////////////////

#include "nds/ndstypes.h"
#include "nds/arm9/cp15.h"

ITCM_CODE ARM_CODE static inline
uintptr_t align_up(const void *address, size_t size)
{
    size_t mask = size - 1;
    return ((uintptr_t)address + mask) & ~mask;
}

ITCM_CODE ARM_CODE static inline
uintptr_t align_down(const void *address, size_t size)
{
    size_t mask = size - 1;
    return (uintptr_t)address & ~mask;
}

// For functions that work with memory ranges:
// - Align start to the cache line size, rounded down to include the base.
// - Align end to the cache line size too, but round up to include the end.

ITCM_CODE ARM_CODE
void CP15_CleanAndFlushDcacheRange(const void *base, size_t size)
{
    uintptr_t address = align_down(base, CACHE_LINE_SIZE);
    uintptr_t end = align_up(base + size, CACHE_LINE_SIZE);

    // CP15_CleanAndFlushDCacheEntry
    for ( ; address < end; address += CACHE_LINE_SIZE)
        asm volatile("mcr p15, 0, %0, c7, c14, 1" :: "r" (address) : "memory");

    // Ensure that all entries have been written to external memory.

    // CP15_DrainWriteBuffer
    asm volatile("mov r0, #0\n"
                 "mcr p15, 0, r0, c7, c10, 4" ::: "r0", "memory");
}

ITCM_CODE ARM_CODE
void CP15_FlushDcacheRange(const void *base, size_t size)
{
    uintptr_t address = align_down(base, CACHE_LINE_SIZE);
    uintptr_t end = align_up(base + size, CACHE_LINE_SIZE);

    // CP15_FlushDCacheEntry
    for ( ; address < end; address += CACHE_LINE_SIZE)
        asm volatile("mcr p15, 0, %0, c7, c6, 1" :: "r" (address) : "memory");

    // There is nothing to write to memory.
}

ITCM_CODE ARM_CODE
void CP15_FlushIcacheRange(const void *base, size_t size)
{
    uintptr_t address = align_down(base, CACHE_LINE_SIZE);
    uintptr_t end = align_up(base + size, CACHE_LINE_SIZE);

    // CP15_FlushICacheEntry
    for ( ; address < end; address += CACHE_LINE_SIZE)
        asm volatile("mcr p15, 0, %0, c7, c5, 1" :: "r" (address) : "memory");

    // There is nothing to write to memory.
}
