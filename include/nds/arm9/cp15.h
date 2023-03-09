//////////////////////////////////////////////////////////////////////
//
// CP15.h -- CP15 control for the ARM9
//
// version 0.1, February 14, 2005
//
//  Copyright (C) 2005 Michael Noland (joat) and Jason Rogers (dovoto)
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
// Changelog:
//   0.1: First version
//
//////////////////////////////////////////////////////////////////////

#ifndef CP15_INCLUDE
#define CP15_INCLUDE

//////////////////////////////////////////////////////////////////////

#ifndef ARM9
#error CP15 is only for the ARM9
#endif

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////

// Flush functions invalidate cache entries. Clean functions force the memory to
// be updated to the contents of the cache

//////////////////////////////////////////////////////////////////////

#define ICACHE_SIZE           0x2000
#define DCACHE_SIZE           0x1000
#define CACHE_LINE_SIZE       32

//////////////////////////////////////////////////////////////////////

#define CPUID_IMPLEMENTOR(id) ((id) >> 24)          // 0x41
#define CPUID_ARCH(id)        (((id) >> 16) & 0xF)  // 0x04
#define CPUID_PART(id)        (((id) >> 4) & 0xFFF) // 0x946
#define CPUID_VERSION(id)     ((id) & 0xF)          // Revision

uint32_t CP15_GetID(void);

//////////////////////////////////////////////////////////////////////

uint32_t CP15_GetCacheType(void);

//////////////////////////////////////////////////////////////////////

uint32_t CP15_GetTCMSize(void);

//////////////////////////////////////////////////////////////////////

uint32_t CP15_GetControl(void);
void CP15_SetControl(uint32_t data);

//////////////////////////////////////////////////////////////////////

uint32_t CP15_GetDataCachable(void);
uint32_t CP15_GetInstructionCachable(void);
void CP15_SetDataCachable(void);
void CP15_SetInstructionCachable(void);
uint32_t CP15_GetDataBufferable(void);
void CP15_SetDataBufferable(void);

//////////////////////////////////////////////////////////////////////

uint32_t CP15_GetDataPermissions(void);
uint32_t CP15_GetInstructionPermissions(void);
void CP15_SetDataPermissions(void);
void CP15_SetInstructionPermissions(void);

//////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////

void CP15_WaitForInterrupt(void);

//////////////////////////////////////////////////////////////////////

uint32_t CP15_GetDCacheLockdown(void);
uint32_t CP15_GetICacheLockdown(void);
void CP15_SetDCacheLockdown(uint32_t data);
void CP15_SetICacheLockdown(uint32_t data);

//////////////////////////////////////////////////////////////////////

uint32_t CP15_GetDTCM(void);
uint32_t CP15_GetITCM(void);
void CP15_SetDTCM(uint32_t data);
void CP15_SetITCM(uint32_t data);

//////////////////////////////////////////////////////////////////////
void CP15_ITCMEnableDefault(void);

//////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////

#endif // CP15_INCLUDE

//////////////////////////////////////////////////////////////////////
