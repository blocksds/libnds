// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <stddef.h>

#include <nds.h>
#include <nds/ndma.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>

#define NDMA_CHANNEL 1

static u32 sdmmcReadSectors(const u8 devNum, u32 sect, u8 *buf, u32 count) {
#ifdef NDMA_CHANNEL
    if (!(((uintptr_t) buf) & 0x3)) {
        NDMA_SRC(NDMA_CHANNEL) = (u32) getTmioFifo(getTmioRegs(0));
        NDMA_DEST(NDMA_CHANNEL) = (u32) buf;
        NDMA_BLENGTH(NDMA_CHANNEL) = 512 / 4;
        NDMA_BDELAY(NDMA_CHANNEL) = NDMA_BDELAY_DIV_1 | NDMA_BDELAY_CYCLES(0);
        NDMA_CR(NDMA_CHANNEL) = NDMA_ENABLE | NDMA_REPEAT | NDMA_BLOCK_SCALER(4)
                                | NDMA_SRC_FIX | NDMA_DST_INC | NDMA_START_SDMMC;
        u32 result = SDMMC_readSectors(devNum, sect, NULL, count);
        NDMA_CR(NDMA_CHANNEL) = 0;
        return result;
    } else
#endif
    return SDMMC_readSectors(devNum, sect, buf, count);
}

static u32 sdmmcWriteSectors(const u8 devNum, u32 sect, const u8 *buf, u32 count) {
#ifdef NDMA_CHANNEL
    if (!(((uintptr_t) buf) & 0x3)) {
        NDMA_SRC(NDMA_CHANNEL) = (u32) buf;
        NDMA_DEST(NDMA_CHANNEL) = (u32) getTmioFifo(getTmioRegs(0));
        NDMA_BLENGTH(NDMA_CHANNEL) = 512 / 4;
        NDMA_BDELAY(NDMA_CHANNEL) = NDMA_BDELAY_DIV_1 | NDMA_BDELAY_CYCLES(0);
        NDMA_CR(NDMA_CHANNEL) = NDMA_ENABLE | NDMA_REPEAT | NDMA_BLOCK_SCALER(4)
                                | NDMA_SRC_INC | NDMA_DST_FIX | NDMA_START_SDMMC;
        u32 result = SDMMC_writeSectors(devNum, sect, NULL, count);
        NDMA_CR(NDMA_CHANNEL) = 0;
        return result;
    } else
#endif
    return SDMMC_writeSectors(devNum, sect, buf, count);
}

int sdmmcMsgHandler(int bytes, void *user_data, FifoMessage *msg)
{
    (void)bytes;
    (void)user_data;

    int retval = 0;

    switch (msg->type)
    {
        case SDMMC_SD_READ_SECTORS:
			retval = sdmmcReadSectors(SDMMC_DEV_CARD, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors);
            break;
        case SDMMC_SD_WRITE_SECTORS:
			retval = sdmmcWriteSectors(SDMMC_DEV_CARD, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors);
            break;
        case SDMMC_NAND_READ_SECTORS:
			retval = sdmmcReadSectors(SDMMC_DEV_eMMC, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors);
            break;
        case SDMMC_NAND_WRITE_SECTORS:
			retval = sdmmcWriteSectors(SDMMC_DEV_eMMC, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors);
            break;
    }

    return retval;
}

int sdmmcValueHandler(u32 value, void *user_data)
{
    (void)user_data;

    int result = 0;

    switch(value)
    {
		case SDMMC_SD_STATUS:
            result = SDMMC_getDiskStatus(SDMMC_DEV_CARD);
            break;

		case SDMMC_NAND_STATUS:
            result = SDMMC_getDiskStatus(SDMMC_DEV_eMMC);
            break;

        case SDMMC_SD_START:
			result = SDMMC_init(SDMMC_DEV_CARD);
			break;

        case SDMMC_NAND_START:
			result = SDMMC_init(SDMMC_DEV_eMMC);
			break;

        case SDMMC_SD_STOP:
			result = SDMMC_deinit(SDMMC_DEV_CARD);
            break;

        case SDMMC_NAND_STOP:
			break;

        case SDMMC_SD_SIZE:
            result = SDMMC_getSectors(SDMMC_DEV_CARD);
            break;

        case SDMMC_NAND_SIZE:
            result = SDMMC_getSectors(SDMMC_DEV_eMMC);
            break;
    }

    return result;
}
