// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

/// @file nds/arm9/tcm_alloc.h
///
/// @brief Dynamic memory allocators for DTCM and ITCM.
///
/// The DTCM allocator is only available if the linker symbol `__dtcm_data_size`
/// has been set to a positive value.
///
/// Both the ITCM and DTCM allocators require a minimum of space that isn't used
/// by static variables and code. They will fail to initialize if there isn't
/// enough space, and all allocation attempts will return NULL.

#ifndef LIBNDS_NDS_ARM9_TCM_ALLOC_H__
#define LIBNDS_NDS_ARM9_TCM_ALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void *ictm_malloc(size_t size);
void *itcm_calloc(size_t n, size_t size);
void *itcm_realloc(void *p, size_t size);
void *itcm_memalign(size_t alignment, size_t size);
void *itcm_aligned_alloc(size_t alignment, size_t size);
void itcm_free(void *ptr);

void *dctm_malloc(size_t size);
void *dtcm_calloc(size_t n, size_t size);
void *dtcm_realloc(void *p, size_t size);
void *dtcm_memalign(size_t alignment, size_t size);
void *dtcm_aligned_alloc(size_t alignment, size_t size);
void dtcm_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_TCM_ALLOC_H__
