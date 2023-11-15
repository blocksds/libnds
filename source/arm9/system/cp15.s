// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)

// CP15 control for the ARM9

#include <nds/asminc.h>
#include <nds/arm9/cp15_asm.h>

    .syntax  unified
    .arch    armv5te
    .cpu     arm946e-s

    .arm

//////////////////////////////////////////////////////////////////////
// See DDI0155A page 2-6
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetID
    mrc     CP15_REG0_ID_CODE_REG(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155A page 2-7
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetCacheType
    mrc     CP15_REG0_CACHE_TYPE(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155A page 2-9
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetTCMSize
    mrc     CP15_REG0_TCM_SIZE(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155A page 2-11
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetControl
    mrc     CP15_REG1_CONTROL_REGISTER(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetControl
    mcr     CP15_REG1_CONTROL_REGISTER(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-15
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDataCachable
    mrc     CP15_REG2_DATA_CACHE_CONFIG(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_GetInstructionCachable
    mrc     CP15_REG2_INSTRUCTION_CACHE_CONFIG(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetDataCachable
    mcr     CP15_REG2_DATA_CACHE_CONFIG(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetInstructionCachable
    mcr     CP15_REG2_INSTRUCTION_CACHE_CONFIG(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-15
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDataBufferable
    mrc     CP15_REG3_WRITE_BUFFER_CONTROL(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetDataBufferable
    mcr     CP15_REG3_WRITE_BUFFER_CONTROL(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-16
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDataPermissions
    mrc     CP15_REG5_DATA_ACCESS_PERMISSION(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_GetInstructionPermissions
    mrc     CP15_REG5_INSTRUCTION_ACCESS_PERMISSION(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetDataPermissions
    mcr     CP15_REG5_DATA_ACCESS_PERMISSION(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetInstructionPermissions
    mcr     CP15_REG5_INSTRUCTION_ACCESS_PERMISSION(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-19
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetRegion0
    mrc     CP15_REG6_PROTECTION_REGION(r0, 0)
    bx      lr

BEGIN_ASM_FUNC CP15_GetRegion1
    mrc     CP15_REG6_PROTECTION_REGION(r0, 1)
    bx      lr

BEGIN_ASM_FUNC CP15_GetRegion2
    mrc     CP15_REG6_PROTECTION_REGION(r0, 2)
    bx      lr

BEGIN_ASM_FUNC CP15_GetRegion3
    mrc     CP15_REG6_PROTECTION_REGION(r0, 3)
    bx      lr

BEGIN_ASM_FUNC CP15_GetRegion4
    mrc     CP15_REG6_PROTECTION_REGION(r0, 4)
    bx      lr

BEGIN_ASM_FUNC CP15_GetRegion5
    mrc     CP15_REG6_PROTECTION_REGION(r0, 5)
    bx      lr

BEGIN_ASM_FUNC CP15_GetRegion6
    mrc     CP15_REG6_PROTECTION_REGION(r0, 6)
    bx      lr

BEGIN_ASM_FUNC CP15_GetRegion7
    mrc     CP15_REG6_PROTECTION_REGION(r0, 7)
    bx      lr


BEGIN_ASM_FUNC CP15_SetRegion0
    mcr     CP15_REG6_PROTECTION_REGION(r0, 0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetRegion1
    mcr     CP15_REG6_PROTECTION_REGION(r0, 1)
    bx      lr

BEGIN_ASM_FUNC CP15_SetRegion2
    mcr     CP15_REG6_PROTECTION_REGION(r0, 2)
    bx      lr

BEGIN_ASM_FUNC CP15_SetRegion3
    mcr     CP15_REG6_PROTECTION_REGION(r0, 3)
    bx      lr

BEGIN_ASM_FUNC CP15_SetRegion4
    mcr     CP15_REG6_PROTECTION_REGION(r0, 4)
    bx      lr

BEGIN_ASM_FUNC CP15_SetRegion5
    mcr     CP15_REG6_PROTECTION_REGION(r0, 5)
    bx      lr

BEGIN_ASM_FUNC CP15_SetRegion6
    mcr     CP15_REG6_PROTECTION_REGION(r0, 6)
    bx      lr

BEGIN_ASM_FUNC CP15_SetRegion7
    mcr     CP15_REG6_PROTECTION_REGION(r0, 7)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-19
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_FlushICache
    mov     r0, #0
    mcr     CP15_REG7_FLUSH_ICACHE
    bx      lr

BEGIN_ASM_FUNC CP15_FlushICacheEntry
    mcr     CP15_REG7_FLUSH_ICACHE_ENTRY(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_PrefetchICacheLine
    mcr     CP15_REG7_PREFETCH_ICACHE_LINE(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_FlushDCache
    mov     r0, #0
    mcr     CP15_REG7_FLUSH_DCACHE
    bx      lr

BEGIN_ASM_FUNC CP15_FlushDCacheEntry
    mcr     CP15_REG7_FLUSH_DCACHE_ENTRY(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_CleanDCacheEntry
    mcr     CP15_REG7_CLEAN_DCACHE_ENTRY(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_CleanAndFlushDCacheEntry
    mcr     CP15_REG7_CLEAN_FLUSH_DCACHE_ENTRY(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_CleanDCacheEntryByIndex
    mcr     CP15_REG7_CLEAN_DCACHE_ENTRY_BY_INDEX(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_CleanAndFlushDCacheEntryByIndex
    mcr     CP15_REG7_CLEAN_FLUSH_DCACHE_ENTRY_BY_INDEX(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-24
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_DrainWriteBuffer
    mov     r0, #0
    mcr     CP15_REG7_DRAIN_WRITE_BUFFER
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-24
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_WaitForInterrupt
    mov     r0, #0
    mcr     CP15_REG7_WAIT_FOR_INTERRUPT
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-25
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDCacheLockdown
    mrc     CP15_REG9_DATA_LOCKDOWN_CONTROL(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_GetICacheLockdown
    mrc     CP15_REG9_INSTRUCTION_LOCKDOWN_CONTROL(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetDCacheLockdown
    mcr     CP15_REG9_DATA_LOCKDOWN_CONTROL(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetICacheLockdown
    mcr     CP15_REG9_INSTRUCTION_LOCKDOWN_CONTROL(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// See DDI0155 page 2-26
//////////////////////////////////////////////////////////////////////

BEGIN_ASM_FUNC CP15_GetDTCM
    mrc     CP15_REG9_DTCM_CONTROL(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_GetITCM
    mrc     CP15_REG9_ITCM_CONTROL(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetDTCM
    mcr     CP15_REG9_DTCM_CONTROL(r0)
    bx      lr

BEGIN_ASM_FUNC CP15_SetITCM
    mcr     CP15_REG9_ITCM_CONTROL(r0)
    bx      lr

//////////////////////////////////////////////////////////////////////
// Routine obtained from page 3-11 of ARM DDI 0201D
//////////////////////////////////////////////////////////////////////
BEGIN_ASM_FUNC CP15_CleanAndFlushDcache

    // Loop in all 4 segments
    mov     r1, #0
outer_loop:

    // Loop in all entries in one segment
    mov     r0, #0
inner_loop:
    orr     r2, r1, r0 // Generate segment and line address
    mcr     CP15_REG7_CLEAN_FLUSH_DCACHE_ENTRY_BY_INDEX(r2) // Clean the line
    add     r0, r0, #CACHE_LINE_SIZE // Increment to next line
    cmp     r0, #(DCACHE_SIZE / ENTRIES_PER_SEGMENT)
    bne     inner_loop

    add     r1, r1, #0x40000000 // Increment segment counter
    cmp     r1, #0x0
    bne     outer_loop

    // Drain write buffer
    mov     r0, #0
    mcr     CP15_REG7_DRAIN_WRITE_BUFFER

    bx lr

    .end
