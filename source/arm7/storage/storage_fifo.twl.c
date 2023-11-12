// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <stddef.h>

#include <nds.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>

int sdmmcMsgHandler(int bytes, void *user_data, FifoMessage *msg)
{
    (void)bytes;
    (void)user_data;

    int retval = 0;

    switch (msg->type)
    {
        case SDMMC_SD_READ_SECTORS:
			retval = SDMMC_readSectors(SDMMC_DEV_CARD, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors);
            break;
        case SDMMC_SD_WRITE_SECTORS:
			retval = SDMMC_writeSectors(SDMMC_DEV_CARD, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors);
            break;
        case SDMMC_NAND_READ_SECTORS:
			retval = SDMMC_readSectors(SDMMC_DEV_eMMC, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors);
            break;
        case SDMMC_NAND_WRITE_SECTORS:
			retval = SDMMC_writeSectors(SDMMC_DEV_eMMC, msg->sdParams.startsector,
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
        case SDMMC_HAVE_SD:
		case SDMMC_SD_IS_INSERTED:
            result = TMIO_cardDetected();
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

        case SDMMC_NAND_SIZE:
            result = SDMMC_getSectors(SDMMC_DEV_eMMC);
            break;
    }

    return result;
}
