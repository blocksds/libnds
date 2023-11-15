// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2023 Antonio Niño Díaz

// CP15 control for the ARM9

#ifndef LIBNDS_NDS_ARM9_CP15_H__
#define LIBNDS_NDS_ARM9_CP15_H__

#ifndef ARM9
#error CP15 is only for the ARM9
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <nds/arm9/cp15_asm.h>

// Flush functions invalidate cache entries. Clean functions force the memory to
// be updated to the contents of the cache

#define CPUID_IMPLEMENTOR(id) ((id) >> 24)          // 0x41
#define CPUID_ARCH(id)        (((id) >> 16) & 0xF)  // 0x04
#define CPUID_PART(id)        (((id) >> 4) & 0xFFF) // 0x946
#define CPUID_VERSION(id)     ((id) & 0xF)          // Revision

uint32_t CP15_GetID(void);

uint32_t CP15_GetCacheType(void);

uint32_t CP15_GetTCMSize(void);

uint32_t CP15_GetControl(void);
void CP15_SetControl(uint32_t data);

uint32_t CP15_GetDataCachable(void);
uint32_t CP15_GetInstructionCachable(void);
void CP15_SetDataCachable(void);
void CP15_SetInstructionCachable(void);
uint32_t CP15_GetDataBufferable(void);
void CP15_SetDataBufferable(void);

uint32_t CP15_GetDataPermissions(void);
uint32_t CP15_GetInstructionPermissions(void);
void CP15_SetDataPermissions(void);
void CP15_SetInstructionPermissions(void);

uint32_t CP15_GetRegion0(void);
uint32_t CP15_GetRegion1(void);
uint32_t CP15_GetRegion2(void);
uint32_t CP15_GetRegion3(void);
uint32_t CP15_GetRegion4(void);
uint32_t CP15_GetRegion5(void);
uint32_t CP15_GetRegion6(void);
uint32_t CP15_GetRegion7(void);
void CP15_SetRegion0(uint32_t data);
void CP15_SetRegion1(uint32_t data);
void CP15_SetRegion2(uint32_t data);
void CP15_SetRegion3(uint32_t data);
void CP15_SetRegion4(uint32_t data);
void CP15_SetRegion5(uint32_t data);
void CP15_SetRegion6(uint32_t data);
void CP15_SetRegion7(uint32_t data);

// Flush entire instruction cache
void CP15_FlushICache(void);

void CP15_FlushICacheEntry(uintptr_t address);
void CP15_PrefetchICacheLine(uintptr_t address);

// Flush entire data cache
void CP15_FlushDCache(void);

void CP15_FlushDCacheEntry(uintptr_t address);
void CP15_CleanDCacheEntry(uintptr_t address);
void CP15_CleanAndFlushDCacheEntry(uintptr_t address);
void CP15_CleanDCacheEntryByIndex(uint32_t index);
void CP15_CleanAndFlushDCacheEntryByIndex(uint32_t index);

// This stalls the processor core until any outstanding accesses in the write
// buffer are completed, that is, until all data is written to external memory.
void CP15_DrainWriteBuffer(void);

void CP15_WaitForInterrupt(void);

uint32_t CP15_GetDCacheLockdown(void);
uint32_t CP15_GetICacheLockdown(void);
void CP15_SetDCacheLockdown(uint32_t data);
void CP15_SetICacheLockdown(uint32_t data);

uint32_t CP15_GetDTCM(void);
uint32_t CP15_GetITCM(void);
void CP15_SetDTCM(uint32_t data);
void CP15_SetITCM(uint32_t data);

// Helper functions

void CP15_CleanAndFlushDcacheRange(const void *base, size_t size);
void CP15_FlushDcacheRange(const void *base, size_t size);
void CP15_CleanAndFlushDcache(void);

void CP15_FlushIcacheRange(const void *base, size_t size);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_CP15_H__
