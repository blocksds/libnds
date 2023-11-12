// SPDX-License-Identifier: Zlib
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

static u32 sdmmc_fifo_value(uint32_t cmd)
{
    u32 result;

    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendValue32(FIFO_STORAGE, cmd);
    fifoWaitValue32Async(FIFO_STORAGE);
    result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result;
}

static u32 sdmmc_fifo_sectors(uint32_t cmd, sec_t sector, sec_t numSectors, void *buffer, bool write)
{
    FifoMessage msg;

    DC_FlushRange(buffer, numSectors * 512);

    msg.type = cmd;
    msg.sdParams.startsector = sector;
    msg.sdParams.numsectors = numSectors;
    msg.sdParams.buffer = buffer;

    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8 *)&msg);

    if (write) {
        DC_InvalidateRange(buffer, numSectors * 512);
        fifoWaitValue32Async(FIFO_STORAGE);
    } else {
        fifoWaitValue32Async(FIFO_STORAGE);
        DC_InvalidateRange(buffer, numSectors * 512);
    }

    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result;
}

bool sdmmc_ClearStatus(void)
{
    return true;
}

bool sdmmc_Shutdown(void)
{
    return true;
}

u8 nand_GetDiskStatus(void)
{
    return sdmmc_fifo_value(SDMMC_NAND_STATUS);
}

u8 sdmmc_GetDiskStatus(void)
{
    return sdmmc_fifo_value(SDMMC_SD_STATUS);
}

u32 nand_GetSectors(void)
{
    return sdmmc_fifo_value(SDMMC_NAND_SIZE);
}

u32 sdmmc_GetSectors(void)
{
    return sdmmc_fifo_value(SDMMC_SD_SIZE);
}

bool nand_Startup(void)
{
    return sdmmc_fifo_value(SDMMC_NAND_START) == 0;
}

bool sdmmc_Startup(void)
{
    if (sdmmc_fifo_value(SDMMC_SD_STATUS) & SDMMC_STATUS_NODISK)
        return false;

    return sdmmc_fifo_value(SDMMC_SD_START) == 0;
}

bool nand_IsInserted(void)
{
    return true;
}

bool sdmmc_IsInserted(void)
{
    return (sdmmc_GetDiskStatus() & SDMMC_STATUS_NODISK) == 0;
}

bool nand_ReadSectors(sec_t sector, sec_t numSectors, void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_NAND_READ_SECTORS, sector, numSectors, buffer, false) == 0;
}

bool sdmmc_ReadSectors(sec_t sector, sec_t numSectors, void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_SD_READ_SECTORS, sector, numSectors, buffer, false) == 0;
}

bool nand_WriteSectors(sec_t sector, sec_t numSectors, const void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_NAND_WRITE_SECTORS, sector, numSectors, (void*) buffer, true) == 0;
}

bool sdmmc_WriteSectors(sec_t sector, sec_t numSectors, const void *buffer)
{
    return sdmmc_fifo_sectors(SDMMC_SD_WRITE_SECTORS, sector, numSectors, (void*) buffer, true) == 0;
}

/* const DISC_INTERFACE __io_dsinand = {
    DEVICE_TYPE_DSI_SD,
    FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
    &nand_Startup,
    &nand_IsInserted,
    &nand_ReadSectors,
    &nand_WriteSectors,
    &sdmmc_ClearStatus,
    &sdmmc_Shutdown
}; */

const DISC_INTERFACE __io_dsisd = {
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
