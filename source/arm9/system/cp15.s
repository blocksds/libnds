// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)

// CP15.S -- CP15 control for the ARM9

#include "nds/asminc.h"

    .arm

//////////////////////////////////////////////////////////////////////
// See DDI0155A page 2-6
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetID
	mrc	p15, 0, r0, c0, c0, 0
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155A page 2-7
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetCacheType
	mrc	p15, 0, r0, c0, c0, 1
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155A page 2-9
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetTCMSize
	mrc	p15, 0, r0, c0, c0, 2
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155A page 2-11
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetControl
	mrc	p15, 0, r0, c1, c0, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetControl
	mcr	p15, 0, r0, c1, c0, 0
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-15
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDataCachable
	mrc	p15, 0, r0, c2, c0, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetInstructionCachable
	mrc	p15, 0, r0, c2, c0, 1
	bx	lr

BEGIN_ASM_FUNC CP15_SetDataCachable
	mcr	p15, 0, r0, c2, c0, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetInstructionCachable
	mcr	p15, 0, r0, c2, c0, 1
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-15
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDataBufferable
	mrc	p15, 0, r0, c3, c0, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetDataBufferable
	mcr	p15, 0, r0, c3, c0, 0
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-16
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDataPermissions
	mrc	p15, 0, r0, c5, c0, 2
	bx	lr

BEGIN_ASM_FUNC CP15_GetInstructionPermissions
	mrc	p15, 0, r0, c5, c0, 3
	bx	lr

BEGIN_ASM_FUNC CP15_SetDataPermissions
	mcr	p15, 0, r0, c5, c0, 2
	bx	lr

BEGIN_ASM_FUNC CP15_SetInstructionPermissions
	mcr	p15, 0, r0, c5, c0, 3
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-19
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetRegion0
	mrc	p15, 0, r0, c6, c0, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetRegion1
	mrc	p15, 0, r0, c6, c1, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetRegion2
	mrc	p15, 0, r0, c6, c2, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetRegion3
	mrc	p15, 0, r0, c6, c3, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetRegion4
	mrc	p15, 0, r0, c6, c4, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetRegion5
	mrc	p15, 0, r0, c6, c5, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetRegion6
	mrc	p15, 0, r0, c6, c6, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetRegion7
	mrc	p15, 0, r0, c6, c7, 0
	bx	lr


BEGIN_ASM_FUNC CP15_SetRegion0
	mcr	p15, 0, r0, c6, c0, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetRegion1
	mcr	p15, 0, r0, c6, c1, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetRegion2
	mcr	p15, 0, r0, c6, c2, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetRegion3
	mcr	p15, 0, r0, c6, c3, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetRegion4
	mcr	p15, 0, r0, c6, c4, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetRegion5
	mcr	p15, 0, r0, c6, c5, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetRegion6
	mcr	p15, 0, r0, c6, c6, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetRegion7
	mcr	p15, 0, r0, c6, c7, 0
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-19
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_FlushICache
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0
	bx	lr

BEGIN_ASM_FUNC CP15_FlushICacheEntry
	mcr	p15, 0, r0, c7, c5, 1
	bx	lr

BEGIN_ASM_FUNC CP15_PrefetchICacheLine
	mcr	p15, 0, r0, c7, c13, 1
	bx	lr

BEGIN_ASM_FUNC CP15_FlushDCache
	mov	r0, #0
	mcr	p15, 0, r0, c7, c6, 0
	bx	lr

BEGIN_ASM_FUNC CP15_FlushDCacheEntry
	mcr	p15, 0, r0, c7, c6, 1
	bx	lr

BEGIN_ASM_FUNC CP15_CleanDCacheEntry
	mcr	p15, 0, r0, c7, c10, 1
	bx	lr

BEGIN_ASM_FUNC CP15_CleanAndFlushDCacheEntry
	mcr	p15, 0, r0, c7, c14, 1
	bx	lr

BEGIN_ASM_FUNC CP15_CleanDCacheEntryByIndex
	mcr	p15, 0, r0, c7, c10, 2
	bx	lr

BEGIN_ASM_FUNC CP15_CleanAndFlushDCacheEntryByIndex
	mcr	p15, 0, r0, c7, c14, 2
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-24
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_DrainWriteBuffer
	mov	r0, #0
	mcr	p15, 0, r0, c7, c10, 4
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-24
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_WaitForInterrupt
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, 4
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-25
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDCacheLockdown
	mrc	p15, 0, r0, c9, c0, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetICacheLockdown
	mrc	p15, 0, r0, c9, c0, 1
	bx	lr

BEGIN_ASM_FUNC CP15_SetDCacheLockdown
	mcr	p15, 0, r0, c9, c0, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetICacheLockdown
	mcr	p15, 0, r0, c9, c0, 1
	bx	lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-26
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDTCM
	mrc	p15, 0, r0, c9, c1, 0
	bx	lr

BEGIN_ASM_FUNC CP15_GetITCM
	mrc	p15, 0, r0, c9, c1, 1
	bx	lr

BEGIN_ASM_FUNC CP15_SetDTCM
	mcr	p15, 0, r0, c9, c1, 0
	bx	lr

BEGIN_ASM_FUNC CP15_SetITCM
	mcr	p15, 0, r0, c9, c1, 1
	bx	lr

//////////////////////////////////////////////////////////////////////

#define DCACHE_SIZE           0x1000
#define CACHE_LINE_SIZE       32
#define ENTRIES_PER_SEGMENT   4

BEGIN_ASM_FUNC CP15_CleanAndFlushDcache

	// Routine obtained from page 3-11 of ARM DDI 0201D

	// Loop in all 4 segments
	mov	r1, #0
outer_loop:

	// Loop in all entries in one segment
	mov	r0, #0
inner_loop:
	orr	r2, r1, r0 // Generate segment and line address
	mcr	p15, 0, r2, c7, c14, 2 // Clean and flush the line
	add	r0, r0, #CACHE_LINE_SIZE // Increment to next line
	cmp	r0, #(DCACHE_SIZE / ENTRIES_PER_SEGMENT)
	bne	inner_loop

	add	r1, r1, #0x40000000 // Increment segment counter
	cmp	r1, #0x0
	bne	outer_loop

	// Drain write buffer
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 4

	bx lr

	.end
