// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#include <nds/arm9/cp15.h>
#include <nds/ndstypes.h>

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
void CP15_CleanAndFlushDCacheRange(const void *base, size_t size)
{
    uintptr_t address = align_down(base, CACHE_LINE_SIZE);
    uintptr_t end = align_up((const char *)base + size, CACHE_LINE_SIZE);

    // CP15_CleanAndFlushDCacheEntry
    for ( ; address < end; address += CACHE_LINE_SIZE)
        asm volatile("mcr p15, 0, %0, c7, c14, 1" :: "r" (address) : "memory");

    // Ensure that all entries have been written to external memory.

    // CP15_DrainWriteBuffer
    asm volatile("mov r0, #0\n"
                 "mcr p15, 0, r0, c7, c10, 4" ::: "r0", "memory");
}

ITCM_CODE ARM_CODE
void CP15_FlushDCacheRange(const void *base, size_t size)
{
    uintptr_t address = align_down(base, CACHE_LINE_SIZE);
    uintptr_t end = align_up((const char *)base + size, CACHE_LINE_SIZE);

    // CP15_FlushDCacheEntry
    for ( ; address < end; address += CACHE_LINE_SIZE)
        asm volatile("mcr p15, 0, %0, c7, c6, 1" :: "r" (address) : "memory");

    // There is nothing to write to memory.
}

ITCM_CODE ARM_CODE
void CP15_FlushICacheRange(const void *base, size_t size)
{
    uintptr_t address = align_down(base, CACHE_LINE_SIZE);
    uintptr_t end = align_up((const char *)base + size, CACHE_LINE_SIZE);

    // CP15_FlushICacheEntry
    for ( ; address < end; address += CACHE_LINE_SIZE)
        asm volatile("mcr p15, 0, %0, c7, c5, 1" :: "r" (address) : "memory");

    // There is nothing to write to memory.
}
