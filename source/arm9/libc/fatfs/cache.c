// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023-2024 Antonio Niño Díaz

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"

typedef struct
{
    uint8_t  valid;
    uint8_t  pdrv;
    LBA_t    sector;
    uint32_t used_at;
} cache_entry_t;

#if FF_MAX_SS != FF_MIN_SS
#error "This code expects a fixed sector size"
#endif

static cache_entry_t *cache_entries = NULL;
static uint8_t *cache_mem;
static uint32_t cache_num_sectors = 0;
static uint32_t dldi_stub_space_sectors;
static uint32_t usage_counter = 0;

extern uint8_t *dldiGetStubDataEnd(void);
extern uint8_t *dldiGetStubEnd(void);

bool cache_initialized(void)
{
    if (cache_entries != NULL)
        return true;
    return false;
}

void cache_deinit(void)
{
    if (cache_entries != NULL)
    {
        free(cache_entries);
        cache_entries = NULL;
    }

    if (cache_mem != NULL)
    {
        free(cache_mem);
        cache_mem = NULL;
    }

    cache_num_sectors = 0;
}

int cache_init(int32_t num_sectors)
{
    // If this function is called after the first time, clear the cache and
    // allocate a new one.
    cache_deinit();

    int32_t stub_space_sectors = (dldiGetStubEnd() - dldiGetStubDataEnd()) >> 9;
    dldi_stub_space_sectors = stub_space_sectors < 0 ? 0 : stub_space_sectors;

    // If num_sectors is negative, use the DLDI stub space.
    if (num_sectors < 0)
    {
        num_sectors = stub_space_sectors;
    }

    if (num_sectors > 0)
    {
        cache_entries = calloc(num_sectors, sizeof(cache_entry_t));
        if (cache_entries == NULL)
            return -1;

#if FF_MAX_SS != FF_MIN_SS
#error "Set the block size to the right value"
#endif

        // cache_mem is only used to store the excess number of sectors
        // that does not otherwise fit in the unused DLDI stub space.
        cache_mem = NULL;
        if (num_sectors > (int32_t)dldi_stub_space_sectors)
        {
            cache_mem = malloc((num_sectors - dldi_stub_space_sectors) * FF_MAX_SS);
            if (cache_mem == NULL)
            {
                free(cache_entries);
                return -1;
            }
        }

        cache_num_sectors = num_sectors;
    }
    else
    {
        cache_num_sectors = 0;
    }

    return 0;
}

static void *cache_sector_address(uint32_t i)
{
    if (i < dldi_stub_space_sectors)
        return dldiGetStubEnd() - (i + 1) * FF_MAX_SS;
    else
        return cache_mem + ((i - dldi_stub_space_sectors) * FF_MAX_SS);
}

void *cache_sector_get(uint8_t pdrv, uint32_t sector)
{
    for (uint32_t i = 0; i < cache_num_sectors; i++)
    {
        cache_entry_t *entry = &(cache_entries[i]);

        if (entry->valid == 0)
            continue;

        if ((entry->pdrv != pdrv) || (entry->sector != sector))
            continue;

        entry->used_at = usage_counter++;

        return cache_sector_address(i);
    }

    return NULL;
}

void *cache_sector_add(uint8_t pdrv, uint32_t sector)
{
    uint32_t used_at_difference = 0;
    uint32_t selected_entry = 0;

    if (!cache_num_sectors)
        return NULL;

    // Assumption: cache_sector_get() has been called,
    // and we know the sector is not present
    for (uint32_t i = 0; i < cache_num_sectors; i++)
    {
        if (cache_entries[i].valid == 0)
        {
            // Entry free, use it
            selected_entry = i;
            break;
        }

        // Check if this entry was least recently used
        uint32_t i_used_at_difference = usage_counter - cache_entries[i].used_at;
        if (i_used_at_difference > used_at_difference)
        {
            used_at_difference = i_used_at_difference;
            selected_entry = i;
        }
    }

    cache_entry_t *entry = &(cache_entries[selected_entry]);

    if (pdrv != 0xFF)
    {
        entry->pdrv = pdrv;
        entry->valid = 1;
        entry->sector = sector;
        entry->used_at = usage_counter++;
    }
    else
    {
        entry->valid = 0;
    }

    return cache_sector_address(selected_entry);
}

void cache_sector_invalidate(uint8_t pdrv, uint32_t sector_from, uint32_t sector_to)
{
    for (uint32_t i = 0; i < cache_num_sectors; i++)
    {
        cache_entry_t *entry = &(cache_entries[i]);

        if (entry->valid == 0)
            continue;

        if ((entry->pdrv != pdrv) || (entry->sector < sector_from) || (entry->sector > sector_to))
            continue;

        entry->valid = 0;
    }
}
