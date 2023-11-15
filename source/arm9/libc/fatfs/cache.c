// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ff.h"

typedef struct {
    uint8_t  valid;
    uint8_t  pdrv;
    LBA_t    sector;
    uint32_t usage_count;
} cache_entry_t;

#if FF_MAX_SS != FF_MIN_SS
#error "This code expects a fixed sector size"
#endif

static cache_entry_t *cache_entries;
static uint8_t *cache_mem;
static uint32_t cache_num_sectors;
static uint32_t dldi_stub_space_sectors;

extern uint8_t* dldiGetStubDataEnd(void);
extern uint8_t* dldiGetStubEnd(void);

int cache_init(int32_t num_sectors)
{
    // If this function is called after the first time, clear the cache and
    // allocate a new one.

    if (cache_entries != NULL)
        free(cache_entries);

    if (cache_mem != NULL)
        free(cache_mem);

    int32_t stub_space_sectors = (dldiGetStubEnd() - dldiGetStubDataEnd()) >> 9;
    dldi_stub_space_sectors = stub_space_sectors < 0 ? 0 : stub_space_sectors;

    // If num_sectors is negative, use the DLDI stub space.
    if (num_sectors < 0) {
        num_sectors = stub_space_sectors;
    }

    if (num_sectors > 0) {
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
    } else {
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

        if (entry->usage_count < cache_num_sectors)
            entry->usage_count++;

        return cache_sector_address(i);
    }

    return NULL;
}

void *cache_sector_add(uint8_t pdrv, uint32_t sector)
{
    int entry_found = 0;
    static uint32_t selected_entry = 0;

    for (uint32_t i = 0; i < cache_num_sectors; i++)
    {
        if (cache_entries[selected_entry].valid == 0)
        {
            entry_found = 1;
            break;
        }

        selected_entry++;
        if (selected_entry >= cache_num_sectors)
            selected_entry = 0;
    }

    // Cache full, evict one entry
    if (entry_found == 0)
    {
        selected_entry++;
        if (selected_entry >= cache_num_sectors)
            selected_entry = 0;

        uint32_t min_usage_count = cache_entries[selected_entry].usage_count;

        if (min_usage_count > 1)
        {
            cache_entries[selected_entry].usage_count--;
            selected_entry++;
            if (selected_entry >= cache_num_sectors)
                selected_entry = 0;

            uint32_t min_entry = selected_entry;

            for (uint32_t i = 1; i < cache_num_sectors; i++)
            {
                cache_entry_t *entry = &(cache_entries[selected_entry]);

                if (entry->usage_count < min_usage_count)
                {
                    min_usage_count = entry->usage_count;
                    min_entry = selected_entry;
                }

                entry->usage_count--;
                if (min_usage_count <= 1)
                    break;

                selected_entry++;
                if (selected_entry >= cache_num_sectors)
                    selected_entry = 0;
            }

            selected_entry = min_entry;
        }
    }

    cache_entry_t *entry = &(cache_entries[selected_entry]);

    entry->pdrv = pdrv;
    entry->valid = 1;
    entry->sector = sector;
    entry->usage_count = 1;

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
        return;
    }
}
