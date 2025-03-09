// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023-2024 Antonio Niño Díaz

#ifndef FATFS_CACHE_H__
#define FATFS_CACHE_H__

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

bool cache_initialized(void);
void cache_deinit(void);
int cache_init(int32_t num_sectors);
void *cache_sector_get(uint8_t pdrv, uint32_t sector);
void *cache_sector_add(uint8_t pdrv, uint32_t sector);
void cache_sector_invalidate(uint8_t pdrv, uint32_t sector_from, uint32_t sector_to);

/**
 * "Borrow" an unused cache entry to use as a write buffer.
 */
__attribute__((always_inline))
static inline void *cache_sector_borrow(void)
{
    return cache_sector_add(0xFF, 0);
}

#endif // FATFS_CACHE_H__
