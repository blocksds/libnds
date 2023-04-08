/*---------------------------------------------------------------------------------
	$Id: cache.h,v 1.8 2008-02-12 00:45:58 wntrmute Exp $

	key input code -- provides slightly higher level input forming

	Copyright (C) 2005
		Michael Noland (joat)
		Jason Rogers (dovoto)
		Dave Murphy (WinterMute)

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.
	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.
	3.	This notice may not be removed or altered from any source
		distribution.

---------------------------------------------------------------------------------*/

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
