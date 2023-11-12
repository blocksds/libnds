/*------------------------------------------------------------------------/
/  Low level disk I/O module SKELETON for FatFs                           /
/-------------------------------------------------------------------------/
/
/ Copyright (C) 2019, ChaN, all right reserved.
/ Copyright (C) 2023, AntonioND, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/----------------------------------------------------------------------------*/

//-----------------------------------------------------------------------
// If a working storage control module is available, it should be
// attached to the FatFs via a glue function rather than modifying it.
// This is an example of glue functions to attach various exsisting
// storage control modules to the FatFs module with a defined API.
//-----------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <nds/arm9/cache.h>
#include <nds/arm9/dldi.h>
#include <nds/arm9/sdmmc.h>
#include <nds/memory.h>
#include <nds/system.h>

#include <fatfs.h>

#include "ff.h"     // Obtains integer types
#include "diskio.h" // Declarations of disk functions
#include "cache.h"

// Definitions of physical drive number for each drive
#define DEV_DLDI    0x00 // DLDI driver (flashcard)
#define DEV_SD      0x01 // SD slot of the DSi

// Disk I/O behaviour configuration
#define IO_CACHE_IGNORE_LARGE_READS 2

// NOTE: The clearStatus() function of DISC_INTERFACE isn't used in libfat, so
// it isn't needed here either.

static bool fs_initialized[FF_VOLUMES];
static const DISC_INTERFACE *fs_io[FF_VOLUMES];

#if FF_MAX_SS != FF_MIN_SS
#error "This file assumes that the sector size is always the same".
#endif

//-----------------------------------------------------------------------
// Get Drive Status
//-----------------------------------------------------------------------

// pdrv: Physical drive nmuber to identify the drive
DSTATUS disk_status(BYTE pdrv)
{
    DSTATUS result = 0;

    switch (pdrv)
    {
        case DEV_SD:
            result = sdmmc_GetDiskStatus();
            // fall through
        case DEV_DLDI:
            result |= fs_initialized[pdrv] ? 0 : STA_NOINIT;
            break;
        default:
            result = STA_NOINIT;
            break;
    }
    
    return result;
}

//-----------------------------------------------------------------------
// Initialize a Drive
//-----------------------------------------------------------------------

// pdrv: Physical drive nmuber to identify the drive
DSTATUS disk_initialize(BYTE pdrv)
{
    // TODO: Should we fail if the device has been initialized, or succeed?
    if (fs_initialized[pdrv])
        return 0;

    switch (pdrv)
    {
        case DEV_DLDI:
        case DEV_SD:
        {
            const DISC_INTERFACE *io = pdrv == DEV_SD ? get_io_dsisd() : dldiGetInternal();

            if (!io->startup())
                return STA_NOINIT;

            if (!io->isInserted())
                return STA_NODISK;

            fs_io[pdrv] = io;
            fs_initialized[pdrv] = true;

            return 0;
        }
    }
    return STA_NOINIT;
}

//-----------------------------------------------------------------------
// Read Sector(s)
//-----------------------------------------------------------------------

// pdrv:   Physical drive nmuber to identify the drive
// buff:   Data buffer to store read data
// sector: Start sector in LBA
// count:  Number of sectors to read
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (!fs_initialized[pdrv])
        return RES_NOTRDY;

    switch (pdrv)
    {
        case DEV_DLDI:
        case DEV_SD:
        {
            const DISC_INTERFACE *io = fs_io[pdrv];
#ifdef IO_CACHE_IGNORE_LARGE_READS
            if (count >= IO_CACHE_IGNORE_LARGE_READS)
            {
                // Is the target in main RAM?
                if (((uint32_t) buff) >> 24 == 0x02 && !(((uint32_t) buff) & 0x03))
                {
                    if (!io->readSectors(sector, count, buff))
                        return RES_ERROR;

                    return RES_OK;
                }
            }
#endif
            while (count > 0)
            {
                void *cache = cache_sector_get(pdrv, sector);
                if (cache != NULL)
                {
                    memcpy(buff, cache, FF_MAX_SS);
                }
                else
                {
                    void *cache = cache_sector_add(pdrv, sector);

                    if (!io->readSectors(sector, 1, cache))
                        return RES_ERROR;

                    memcpy(buff, cache, FF_MAX_SS);
                }

                count--;
                sector++;
                buff += FF_MAX_SS;
            }

            return RES_OK;
        }
    }

    return RES_PARERR;
}

//-----------------------------------------------------------------------
// Write Sector(s)
//-----------------------------------------------------------------------

#if FF_FS_READONLY == 0

// pdrv:   Physical drive nmuber to identify the drive
// buff:   Data to be written
// sector: Start sector in LBA
// count:  Number of sectors to write
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    uint8_t *align_buffer;
    if (fs_initialized[pdrv] == 0)
        return RES_NOTRDY;

    switch (pdrv)
    {
        case DEV_DLDI:
        case DEV_SD:
        {
            for (uint32_t i = 0; i < count; i++)
                cache_sector_invalidate(pdrv, sector + i);

            const DISC_INTERFACE *io = fs_io[pdrv];
            if (((uint32_t) buff) & 0x03)
            {
                // DLDI drivers expect a 4-byte aligned buffer.
                align_buffer = malloc(FF_MAX_SS);
                if (align_buffer == NULL)
                    return RES_ERROR;

                while (count > 0)
                {
                    memcpy(align_buffer, buff, FF_MAX_SS);
                    if (!io->writeSectors(sector, 1, align_buffer))
                        return RES_ERROR;

                    count--;
                    sector++;
                    buff += FF_MAX_SS;
                }

                free(align_buffer);
            }
            else
            {
                if (!io->writeSectors(sector, count, buff))
                    return RES_ERROR;
            }

            return RES_OK;
        }
    }

    return RES_PARERR;
}

#endif

//-----------------------------------------------------------------------
// Miscellaneous Functions
//-----------------------------------------------------------------------

// pdrv: Physical drive nmuber (0..)
// cmd:  Control code
// buff: Buffer to send/receive control data
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)buff;

    if (!fs_initialized[pdrv])
        return RES_NOTRDY;

    // Only CTRL_SYNC is needed for now:
    // - GET_SECTOR_COUNT: Used by f_mkfs and f_fdisk.
    // - GET_SECTOR_SIZE: Required only if FF_MAX_SS > FF_MIN_SS.
    // - GET_BLOCK_SIZE: Used by f_mkfs.
    // - CTRL_TRIM: Required when FF_USE_TRIM == 1.

    switch (pdrv)
    {
        case DEV_DLDI:
        case DEV_SD:
            // This command flushes the cache, but there is no cache right now
            if (cmd == CTRL_SYNC)
                return RES_OK;

            return RES_PARERR;

        default:
            return RES_PARERR;
    }
}

DWORD get_fattime(void)
{
    time_t t = time(0);
    struct tm *stm = localtime(&t);

    return (DWORD)(stm->tm_year - 80) << 25 |
           (DWORD)(stm->tm_mon + 1) << 21 |
           (DWORD)stm->tm_mday << 16 |
           (DWORD)stm->tm_hour << 11 |
           (DWORD)stm->tm_min << 5 |
           (DWORD)stm->tm_sec >> 1;
}
