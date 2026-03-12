// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011-2017 Dave Murphy (WinterMute)

#include <stdbool.h>
#include <nds/arm9/cache.h>
#include <nds/arm9/sdmmc.h>
#include <nds/disc_io.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/memory.h>
#include <nds/system.h>

static u32 TWL_FUNC(sdmmc_fifo_value)(uint32_t cmd)
{
    u32 result;

    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendValue32(FIFO_STORAGE, cmd);
    fifoWaitValue32Async(FIFO_STORAGE);
    result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result;
}

static u32 TWL_FUNC(sdmmc_fifo_sectors)(uint32_t cmd, sec_t sector, sec_t numSectors, void *buffer)
{
    FifoMessage msg;

    DC_FlushRange(buffer, numSectors * 512);

    msg.type = cmd;
    msg.sdParams.startsector = sector;
    msg.sdParams.numsectors = numSectors;
    msg.sdParams.buffer = buffer;

    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32Async(FIFO_STORAGE);
    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result;
}

bool TWL_FUNC(sdmmc_ClearStatus)(void)
{
    return true;
}

bool TWL_FUNC(sdmmc_Shutdown)(void)
{
    return true;
}

u8 TWL_FUNC(sdmmc_GetDiskStatus)(void)
{
    return sdmmc_fifo_value(SDMMC_SD_STATUS);
}

u32 TWL_FUNC(nand_GetSectors)(void)
{
    return sdmmc_fifo_value(SDMMC_NAND_SIZE);
}

u32 TWL_FUNC(sdmmc_GetSectors)(void)
{
    return sdmmc_fifo_value(SDMMC_SD_SIZE);
}

bool TWL_FUNC(nand_Startup)(void)
{
    return sdmmc_fifo_value(SDMMC_NAND_START) == 0;
}

bool TWL_FUNC(nand_SetupCrypt)(void)
{
    return sdmmc_fifo_value(SDMMC_NAND_CRYPT_SETUP) == 0;
}

bool TWL_FUNC(sdmmc_Startup)(void)
{
    if (sdmmc_fifo_value(SDMMC_SD_STATUS) & SDMMC_STATUS_NODISK)
        return false;

    return sdmmc_fifo_value(SDMMC_SD_START) == 0;
}

bool TWL_FUNC(sdmmc_IsInserted)(void)
{
    return (sdmmc_GetDiskStatus() & SDMMC_STATUS_NODISK) == 0;
}

bool TWL_FUNC(nand_ReadSectors)(sec_t sector, sec_t numSectors, void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_NAND_READ_SECTORS, sector, numSectors, buffer) == 0;
}

bool TWL_FUNC(nand_ReadSectorsCrypt)(sec_t sector, sec_t numSectors, void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_NAND_READ_ENCRYPTED_SECTORS, sector, numSectors, buffer) == 0;
}

bool TWL_FUNC(sdmmc_ReadSectors)(sec_t sector, sec_t numSectors, void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_SD_READ_SECTORS, sector, numSectors, buffer) == 0;
}

bool TWL_FUNC(nand_WriteSectors)(sec_t sector, sec_t numSectors, const void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_NAND_WRITE_SECTORS, sector, numSectors, (void *) buffer) == 0;
}

bool TWL_FUNC(nand_WriteSectorsCrypt)(sec_t sector, sec_t numSectors, const void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_NAND_WRITE_ENCRYPTED_SECTORS, sector, numSectors, (void *) buffer) == 0;
}

bool TWL_FUNC(sdmmc_WriteSectors)(sec_t sector, sec_t numSectors, const void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_SD_WRITE_SECTORS, sector, numSectors, (void *) buffer) == 0;
}

const DISC_INTERFACE TWL_DATA_VAR(__io_dsisd) =
{
    DEVICE_TYPE_DSI_SD,
    FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
    &sdmmc_Startup,
    &sdmmc_IsInserted,
    &sdmmc_ReadSectors,
    &sdmmc_WriteSectors,
    &sdmmc_ClearStatus,
    &sdmmc_Shutdown
};

const DISC_INTERFACE *get_io_dsisd(void)
{
    return (isDSiMode() && __NDSHeader->unitCode) ? &__io_dsisd : NULL;
}
