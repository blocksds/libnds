// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Edoardo Lolletti (edo9300)

#include <nds/disc_io.h>
#include <nds/arm9/sdmmc.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/system.h>
#include "ff.h"     // Obtains integer types
#include "diskio.h"

TWL_DATA static bool write_protect = true;

TWL_CODE static u32 sdmmc_fifo_value(uint32_t cmd)
{
    u32 result;

    fifoMutexAcquire(FIFO_STORAGE);

    fifoSendValue32(FIFO_STORAGE, cmd);
    fifoWaitValue32Async(FIFO_STORAGE);
    result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result;
}

TWL_CODE void nand_WriteProtect(bool protect)
{
    write_protect = protect;
}

TWL_CODE bool nandfs_Startup(void)
{
    if (!nand_Startup() || !nand_SetupCrypt())
        return false;

    return true;
}

TWL_CODE u8 nand_GetDiskStatus(void)
{
    return sdmmc_fifo_value(SDMMC_NAND_STATUS) | (write_protect * STA_PROTECT);
}

TWL_CODE static bool nandfs_IsInserted(void)
{
    return true;
}

TWL_CODE static bool nandfs_WriteSectors(sec_t sector, sec_t numSectors, const void *buffer)
{
    if (write_protect)
    {
        return false;
    }
    return nand_WriteSectorsCrypt(sector, numSectors, buffer);
}

TWL_CODE static bool nandfs_ClearStatus(void)
{
    return true;
}

TWL_CODE static bool nandfs_Shutdown(void)
{
    return true;
}

TWL_DATA static DISC_INTERFACE __io_dsinand = {
    DEVICE_TYPE_DSI_NAND,
    FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
    &nandfs_Startup,
    &nandfs_IsInserted,
    &nand_ReadSectorsCrypt,
    &nandfs_WriteSectors,
    &nandfs_ClearStatus,
    &nandfs_Shutdown
};

const DISC_INTERFACE *get_io_dsinand(void)
{
    return isDSiMode() ? &__io_dsinand : NULL;
}
