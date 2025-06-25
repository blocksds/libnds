/*------------------------------------------------------------------------/
/  Low level disk I/O module SKELETON for FatFs                           /
/-------------------------------------------------------------------------/
/
/ Copyright (C) 2019, ChaN, all right reserved.
/ Copyright (C) 2023, Antonio Niño Díaz, all right reserved.
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <aeabi.h>
#include <nds/arm9/cache.h>
#include <nds/arm9/dldi.h>
#include <nds/arm9/sassert.h>
#include <nds/arm9/sdmmc.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/system.h>

#include "../fatfs_internal.h"

#include "ff.h"     // Obtains integer types
#include "diskio.h" // Declarations of disk functions
#include "cache.h"

// Definitions of physical drive number for each drive
#define DEV_DLDI    0x00 // DLDI driver (flashcard)
#define DEV_SD      0x01 // SD slot of the DSi

// Debugging defines.
// #define DISABLE_DIRECT_READS
// #define DISABLE_DIRECT_WRITES
// #define FORCE_CACHE_ALL
// #define FORCE_CACHE_NONE

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
    const DISC_INTERFACE *io;
    DSTATUS result = 0;

    switch (pdrv)
    {
        case DEV_SD:
            result = sdmmc_GetDiskStatus();
            // Fall through
        case DEV_DLDI:
            io = pdrv == DEV_SD ? get_io_dsisd() : dldiGetInternal();
            result |= (io->features & FEATURE_MEDIUM_CANREAD)
                ? ((io->features & FEATURE_MEDIUM_CANWRITE) ? 0 : STA_PROTECT)
                : STA_NODISK;
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

    // Under some conditions, the ARM9 code will yield, so interrupts must be
    // enabled for the yield to be able to finish.
    sassert(REG_IME != 0, "IRQs must be enabled");

    switch (pdrv)
    {
        case DEV_DLDI:
        case DEV_SD:
        {
            const DISC_INTERFACE *io = pdrv == DEV_SD ? get_io_dsisd() : dldiGetInternal();

            if (!(io->features & FEATURE_MEDIUM_CANREAD))
                return STA_NOINIT | STA_NODISK;

            if (!io->startup())
                return STA_NOINIT;

            if (!io->isInserted())
                return STA_NODISK;

            fs_io[pdrv] = io;
            fs_initialized[pdrv] = true;

            return disk_status(pdrv);
        }
    }
    return STA_NOINIT;
}

#define IS_WORD_ALIGNED(buff) (!(((uintptr_t) (buff)) & 0x03))

//-----------------------------------------------------------------------
// Read Sector(s)
//-----------------------------------------------------------------------

// pdrv:   Physical drive nmuber to identify the drive
// buff:   Data buffer to store read data
// sector: Start sector in LBA
// count:  Number of sectors to read
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
#if defined(FORCE_CACHE_NONE)
    bool cacheable = false;
#elif defined(FORCE_CACHE_ALL)
    bool cacheable = true;
#else
    bool cacheable = (pdrv & 0x80);
#endif
    pdrv &= 0x7F;

    if (!fs_initialized[pdrv])
        return RES_NOTRDY;

    sassert(REG_IME != 0, "IRQs must be enabled");

    switch (pdrv)
    {
        case DEV_DLDI:
        case DEV_SD:
        {
            const DISC_INTERFACE *io = fs_io[pdrv];

#ifndef DISABLE_DIRECT_READS
            // The DSi SD driver supports unaligned buffers; we cannot make
            // the same guarantee for DLDI in practice.
            if (!cacheable && memBufferIsInMainRam(buff, count << 9)
                && (pdrv == DEV_SD || IS_WORD_ALIGNED(buff)))
            {
                if (!io->readSectors(sector, count, buff))
                    return RES_ERROR;

                return RES_OK;
            }
#endif

            if (!cacheable)
            {
                void *cache = cache_sector_borrow();

                while (count > 0)
                {
                    if (!io->readSectors(sector, 1, cache))
                    {
                        return RES_ERROR;
                    }

                    __aeabi_memcpy(buff, cache, FF_MAX_SS);

                    count--;
                    sector++;
                    buff += FF_MAX_SS;
                }
            }
            else
            {
                while (count > 0)
                {
                    void *cache = cache_sector_get(pdrv, sector);

                    if (cache == NULL)
                    {
                        cache = cache_sector_add(pdrv, sector);

                        if (!io->readSectors(sector, 1, cache))
                        {
                            cache_sector_invalidate(pdrv, sector, sector);
                            return RES_ERROR;
                        }
                    }

                    __aeabi_memcpy(buff, cache, FF_MAX_SS);

                    count--;
                    sector++;
                    buff += FF_MAX_SS;
                }
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
    if (fs_initialized[pdrv] == 0)
        return RES_NOTRDY;

    sassert(REG_IME != 0, "IRQs must be enabled");

    switch (pdrv)
    {
        case DEV_DLDI:
        case DEV_SD:
        {
            cache_sector_invalidate(pdrv, sector, sector + count - 1);

            const DISC_INTERFACE *io = fs_io[pdrv];
            // The DSi SD driver supports unaligned buffers; we cannot make
            // the same guarantee for DLDI in practice.
#ifndef DISABLE_DIRECT_WRITES
            if (!memBufferIsInMainRam(buff, count << 9)
                || !(pdrv == DEV_SD || IS_WORD_ALIGNED(buff)))
#endif
            {
                // DLDI drivers expect a 4-byte aligned buffer.
                uint8_t *align_buffer = cache_sector_borrow();

                while (count > 0)
                {
                    __aeabi_memcpy(align_buffer, buff, FF_MAX_SS);
                    if (!io->writeSectors(sector, 1, align_buffer))
                    {
                        free(align_buffer);
                        return RES_ERROR;
                    }

                    count--;
                    sector++;
                    buff += FF_MAX_SS;
                }
            }
#ifndef DISABLE_DIRECT_WRITES
            else
            {
                if (!io->writeSectors(sector, count, buff))
                    return RES_ERROR;
            }
#endif

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

    // - CTRL_SYNC: Used for write flush operations.
    // - GET_SECTOR_COUNT: Used by f_mkfs and f_fdisk.
    // - GET_SECTOR_SIZE: Required only if FF_MAX_SS > FF_MIN_SS.
    // - GET_BLOCK_SIZE: Used by f_mkfs.
    // - CTRL_TRIM: Required when FF_USE_TRIM == 1.

    switch (pdrv)
    {
        case DEV_SD:
            if (cmd == GET_SECTOR_COUNT)
            {
                *((LBA_t*) buff) = sdmmc_GetSectors();
                return RES_OK;
            }

            // Fall through

        case DEV_DLDI:
            // This command flushes the writeback cache, but there is no such cache right now.
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

    return fatfs_timestamp_to_fattime(stm);
}
