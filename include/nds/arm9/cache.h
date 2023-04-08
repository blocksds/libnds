// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file cache.h
/// @brief ARM9 cache control functions.

#ifndef _cache_h_
#define _cache_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "nds/ndstypes.h"

#include "nds/arm9/cp15.h"

/// Invalidate the entire instruction cache.
static inline void IC_InvalidateAll(void)
{
    CP15_FlushICache();
}

/// Invalidate the instruction cache of a range of addresses.
///
/// @param base Base address of the region.
/// @param size Size of the region.
static inline void IC_InvalidateRange(const void *base, u32 size)
{
    CP15_FlushIcacheRange(base, size);
}

/// Flush the entire data cache to memory.
static inline void DC_FlushAll(void)
{
    CP15_CleanAndFlushDcache();
}

/// Flush the data cache of a range of addresses to memory.
///
/// @param base Base address of the region.
/// @param size Size of the region.
static inline void DC_FlushRange(const void *base, u32 size)
{
    CP15_CleanAndFlushDcacheRange(base, size);
}

/// Invalidate the entire data cache.
static inline void DC_InvalidateAll(void)
{
    CP15_FlushDCache();
}

/// Invalidate the data cache of a range of addresses.
/// @param base base address of the region to invalidate
/// @param size size of the region to invalidate.
static inline void DC_InvalidateRange(const void *base, u32 size)
{
    CP15_FlushDcacheRange(base, size);
}

#ifdef __cplusplus
}
#endif

#endif
