// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Edoardo Lolletti (edo9300)

#include <nds/disc_io.h>
#include <nds/arm9/sdmmc.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/system.h>

#include "../filesystem_includes.h"

static bool TWL_DATA_VAR(write_protect) = true;

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

void TWL_FUNC(nand_WriteProtect)(bool protect)
{
    write_protect = protect;
}

bool TWL_FUNC(nandfs_Startup)(void)
{
    if (!nand_Startup() || !nand_SetupCrypt())
        return false;

    return true;
}

u8 TWL_FUNC(nand_GetDiskStatus)(void)
{
    return sdmmc_fifo_value(SDMMC_NAND_STATUS) | (write_protect * STA_PROTECT);
}

static bool TWL_FUNC(nandfs_IsInserted)(void)
{
    return true;
}

static bool TWL_FUNC(nandfs_WriteSectors)(sec_t sector, sec_t numSectors, const void *buffer)
{
    if (write_protect)
    {
        return false;
    }
    return nand_WriteSectorsCrypt(sector, numSectors, buffer);
}

static bool TWL_FUNC(nandfs_ClearStatus)(void)
{
    return true;
}

static bool TWL_FUNC(nandfs_Shutdown)(void)
{
    return true;
}

static DISC_INTERFACE TWL_DATA_VAR(__io_dsinand) = {
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
