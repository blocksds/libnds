// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz
// Copyright (C) 2025 Edoardo Lolletti (edo9300)

#include <stddef.h>
#include <string.h>

#include <nds.h>
#include <nds/ndma.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <string.h>
#include <stdio.h>

#define IS_WORD_ALIGNED(buff) (!(((uintptr_t) (buff)) & 0x3))

#define NDMA_CHANNEL 1
#define SECTOR_CAP 2047

static sec_t remainingSectors;
static sec_t startingSector;

#define SECTOR_SIZE             0x200
#define AES_BLOCK_SIZE          16

static sec_t setupAesRegs(u32 sectorNum, sec_t totalSectors)
{
    REG_AES_CNT = AES_CNT_MODE(2) |
                  AES_WRFIFO_FLUSH |
                  AES_RDFIFO_FLUSH |
                  // Apply keyslot 3 containing the nand normal key
                  AES_CNT_KEY_APPLY | AES_CNT_KEYSLOT(3) |
                  // Set both input and output expected dma size to 16 words
                  AES_CNT_DMA_WRITE_SIZE(0) | AES_CNT_DMA_READ_SIZE(3);

    // The blkcnt register holds the number of total blocks (16 bytes) to be parsed
    // by the current aes operation
    sec_t toReadSectors = totalSectors;
    if (toReadSectors > SECTOR_CAP)
    {
        toReadSectors = SECTOR_CAP;
    }
    u32 aesBlockCount = toReadSectors * (SECTOR_SIZE / AES_BLOCK_SIZE);
    REG_AES_BLKCNT = aesBlockCount << 16;

    u32 offset = sectorNum * (SECTOR_SIZE / AES_BLOCK_SIZE);
    // The ctr is the base ctr calculated by the sha of the CID + (address / 16)
    // the aes engine will take care of incrementing it automatically
    nandCrypt_SetIV(offset);

    REG_AES_CNT |= AES_CNT_ENABLE;

    return totalSectors - toReadSectors;
}

static void cryptSectorsRead(u32 fifo, void *buffer, u32 numBytes)
{
    const bool word_aligned = IS_WORD_ALIGNED(buffer);
    vu32 *inSdmcFifo32 = (vu32*)fifo;
    if (word_aligned)
#ifdef NDMA_CHANNEL
    {
        for (unsigned int i = 0; i < numBytes / 4; ++i)
        {
            while (((REG_AES_CNT) & 0x1F) == 16);
            REG_AES_WRFIFO = *inSdmcFifo32;
        }
    }
#else
    {
        vu32 *out32 = (vu32*)buffer;
        for (unsigned int i = 0; i < numBytes / (4 * 16); ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                REG_AES_WRFIFO = *inSdmcFifo32;
            }

            while (((REG_AES_CNT >> 0x5) & 0x1F) < 16);

            for (int j = 0; j < 16; ++j)
            {
                *out32++ = REG_AES_RDFIFO;
            }
        }
    }
#endif
    else
    {
        vu8 *out8 = (vu8*)buffer;
        for (unsigned int i = 0; i < numBytes / (4 * 16); ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                REG_AES_WRFIFO = *inSdmcFifo32;
            }

            while (((REG_AES_CNT >> 0x5) & 0x1F) < 16);

            for (int j = 0; j < 16; ++j)
            {
                const u32 tmp = REG_AES_RDFIFO;
                *out8++ = tmp;
                *out8++ = tmp >> 8;
                *out8++ = tmp >> 16;
                *out8++ = tmp >> 24;
            }
        }
    }
}

static void cryptSectorsWrite(u32 fifo, void *buffer, u32 numBytes)
{
#ifdef NDMA_CHANNEL
    (void)fifo;
    if (IS_WORD_ALIGNED(buffer))
    {
        vu32 *in32 = (vu32*)buffer;
        for (unsigned int i = 0; i < numBytes / 4; ++i)
        {
            while (((REG_AES_CNT) & 0x1F) == 16);

            REG_AES_WRFIFO = *in32++;
        }
    }
    else
    {
        vu8 *in8 = (vu8*)buffer;
        for (unsigned int i = 0; i < numBytes / 4; ++i)
        {
            u32 tmp = *in8++;
            tmp |= *in8++ << 8;
            tmp |= *in8++ << 16;
            tmp |= *in8++ << 24;

            while (((REG_AES_CNT) & 0x1F) == 16);

            REG_AES_WRFIFO = tmp;
        }
    }
#else
    vu32 *outSdmcFifo32 = (vu32*)fifo;
    if (IS_WORD_ALIGNED(buffer))
    {
        vu32 *in32 = (vu32*)buffer;
        for (unsigned int i = 0; i < numBytes / (4 * 16); ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                REG_AES_WRFIFO = *in32++;
            }

            while (((REG_AES_CNT >> 0x5) & 0x1F) < 16);

            for (int j = 0; j < 16; ++j)
            {
                *outSdmcFifo32 = REG_AES_RDFIFO;
            }
        }
    }
    else
    {
        vu8 *in8 = (vu8*)buffer;
        for (unsigned int i = 0; i < numBytes / (4 * 16); ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                u32 tmp = *in8++;
                tmp |= *in8++ << 8;
                tmp |= *in8++ << 16;
                tmp |= *in8++ << 24;
                REG_AES_WRFIFO = tmp;
            }

            while (((REG_AES_CNT >> 0x5) & 0x1F) < 16);

            for (int j = 0; j < 16; ++j)
            {
                *outSdmcFifo32 = REG_AES_RDFIFO;
            }
        }
    }
#endif
}

static void sector_crypt_callback(u32 fifo, void *buffer, u32 numBytes, bool read)
{
    if (read)
        cryptSectorsRead(fifo, buffer, numBytes);
    else
        cryptSectorsWrite(fifo, buffer, numBytes);

    if (remainingSectors != 0)
    {
        u32 cnt = REG_AES_CNT;
        if ((cnt & AES_CNT_ENABLE) == 0)
        {
            startingSector += SECTOR_CAP;
            remainingSectors = setupAesRegs(startingSector, remainingSectors);
#ifdef NDMA_CHANNEL
            if (read)
            {
                if (!IS_WORD_ALIGNED(buffer))
                    return;
                REG_NDMA_DEST(NDMA_CHANNEL) = ((u32)buffer) + 512;
            }
            REG_NDMA_CR(NDMA_CHANNEL) |= NDMA_ENABLE;
#endif
        }
    }
}

static u32 sdmmcReadSectors(const u8 devNum, u32 sect, u8 *buf, u32 count, bool crypt)
{
    u32 result;
    const bool word_aligned = IS_WORD_ALIGNED(buf);

    if (crypt && !nandCrypt_Initialized())
        return SDMMC_ERR_LOCKED;

#ifdef NDMA_CHANNEL
    if (crypt)
    {
        if (word_aligned)
        {
            REG_NDMA_SRC(NDMA_CHANNEL) = (u32)&REG_AES_RDFIFO;
            REG_NDMA_DEST(NDMA_CHANNEL) = (u32)buf;
            REG_NDMA_BLENGTH(NDMA_CHANNEL) = 16;
            REG_NDMA_BDELAY(NDMA_CHANNEL) = NDMA_BDELAY_DIV_1 | NDMA_BDELAY_CYCLES(0);
            REG_NDMA_CR(NDMA_CHANNEL) = NDMA_ENABLE | NDMA_REPEAT | NDMA_BLOCK_SCALER(4)
                                      | NDMA_SRC_FIX | NDMA_DST_INC | NDMA_START_AES_OUT;
        }

        startingSector = sect;
        remainingSectors = setupAesRegs(sect, count);
        result = SDMMC_readSectorsCrypt(devNum, sect, buf, count, sector_crypt_callback);

        if (word_aligned)
        {
            REG_NDMA_CR(NDMA_CHANNEL) = 0;
        }
    }
    else if (word_aligned)
    {
        REG_NDMA_SRC(NDMA_CHANNEL) = (u32) getTmioFifo(getTmioRegs(0));
        REG_NDMA_DEST(NDMA_CHANNEL) = (u32)buf;
        REG_NDMA_BLENGTH(NDMA_CHANNEL) = 512 / 4;
        REG_NDMA_BDELAY(NDMA_CHANNEL) = NDMA_BDELAY_DIV_1 | NDMA_BDELAY_CYCLES(0);
        REG_NDMA_CR(NDMA_CHANNEL) = NDMA_ENABLE | NDMA_REPEAT | NDMA_BLOCK_SCALER(4)
                                  | NDMA_SRC_FIX | NDMA_DST_INC | NDMA_START_SDMMC;
        result = SDMMC_readSectors(devNum, sect, NULL, count);
        REG_NDMA_CR(NDMA_CHANNEL) = 0;
    }
    else
    {
        result = SDMMC_readSectors(devNum, sect, buf, count);
    }
#else
    if (crypt)
    {
        startingSector = sect;
        remainingSectors = setupAesRegs(sect, count);
        result = SDMMC_readSectorsCrypt(devNum, sect, buf, count, sector_crypt_callback);
    }
    else
    {
        result = SDMMC_readSectors(devNum, sect, buf, count);
    }
#endif

    return result;
}

static u32 sdmmcWriteSectors(const u8 devNum, u32 sect, const u8 *buf, u32 count, bool crypt)
{
    u32 result;

    if (crypt && !nandCrypt_Initialized())
        return SDMMC_ERR_LOCKED;

#ifdef NDMA_CHANNEL
    if (crypt)
    {
        REG_NDMA_SRC(NDMA_CHANNEL) = (u32)&REG_AES_RDFIFO;
        REG_NDMA_DEST(NDMA_CHANNEL) = (u32)getTmioFifo(getTmioRegs(0));
        REG_NDMA_BLENGTH(NDMA_CHANNEL) = 16;
        REG_NDMA_BDELAY(NDMA_CHANNEL) = NDMA_BDELAY_DIV_1 | NDMA_BDELAY_CYCLES(0);
        REG_NDMA_CR(NDMA_CHANNEL) = NDMA_ENABLE | NDMA_REPEAT | NDMA_BLOCK_SCALER(4)
                                  | NDMA_SRC_FIX | NDMA_DST_FIX | NDMA_START_AES_OUT;

        startingSector = sect;
        remainingSectors = setupAesRegs(sect, count);
        result = SDMMC_writeSectorsCrypt(devNum, sect, buf, count, sector_crypt_callback);

        REG_NDMA_CR(NDMA_CHANNEL) = 0;
    }
    else if (IS_WORD_ALIGNED(buf))
    {
        REG_NDMA_SRC(NDMA_CHANNEL) = (u32)buf;
        REG_NDMA_DEST(NDMA_CHANNEL) = (u32)getTmioFifo(getTmioRegs(0));
        REG_NDMA_BLENGTH(NDMA_CHANNEL) = 512 / 4;
        REG_NDMA_BDELAY(NDMA_CHANNEL) = NDMA_BDELAY_DIV_1 | NDMA_BDELAY_CYCLES(0);
        REG_NDMA_CR(NDMA_CHANNEL) = NDMA_ENABLE | NDMA_REPEAT | NDMA_BLOCK_SCALER(4)
                                  | NDMA_SRC_INC | NDMA_DST_FIX | NDMA_START_SDMMC;
        result = SDMMC_writeSectors(devNum, sect, NULL, count);
        REG_NDMA_CR(NDMA_CHANNEL) = 0;
    }
    else
    {
        result = SDMMC_writeSectors(devNum, sect, buf, count);
    }
#else
    if (crypt)
    {
        startingSector = sect;
        remainingSectors = setupAesRegs(sect, count);
        result = SDMMC_writeSectorsCrypt(devNum, sect, buf, count, sector_crypt_callback);
    }
    else
    {
        result = SDMMC_writeSectors(devNum, sect, buf, count);
    }
#endif

    return result;
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
                                      msg->sdParams.buffer, msg->sdParams.numsectors, false);
            break;
        case SDMMC_SD_WRITE_SECTORS:
            retval = sdmmcWriteSectors(SDMMC_DEV_CARD, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors, false);
            break;
        case SDMMC_NAND_READ_SECTORS:
            retval = sdmmcReadSectors(SDMMC_DEV_eMMC, msg->sdParams.startsector,
                                      msg->sdParams.buffer, msg->sdParams.numsectors, false);
            break;
        case SDMMC_NAND_WRITE_SECTORS:
            retval = sdmmcWriteSectors(SDMMC_DEV_eMMC, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors, false);
            break;
        case SDMMC_NAND_READ_ENCRYPTED_SECTORS:
            retval = sdmmcReadSectors(SDMMC_DEV_eMMC, msg->sdParams.startsector,
                                      msg->sdParams.buffer, msg->sdParams.numsectors, true);
            break;
        case SDMMC_NAND_WRITE_ENCRYPTED_SECTORS:
            retval = sdmmcWriteSectors(SDMMC_DEV_eMMC, msg->sdParams.startsector,
                                       msg->sdParams.buffer, msg->sdParams.numsectors, true);
            break;
    }

    return retval;
}

int sdmmcValueHandler(u32 value, void *user_data)
{
    (void)user_data;

    int result = 0;

    switch (value)
    {
        case SDMMC_SD_STATUS:
            result = SDMMC_getDiskStatus(SDMMC_DEV_CARD);
            break;

        case SDMMC_NAND_STATUS:
            result = SDMMC_getDiskStatus(SDMMC_DEV_eMMC);
            break;

        case SDMMC_SD_START:
            result = SDMMC_init(SDMMC_DEV_CARD);
            if (result == SDMMC_ERR_INITIALIZED)
                result = SDMMC_ERR_NONE;
            break;

        case SDMMC_NAND_START:
            result = SDMMC_init(SDMMC_DEV_eMMC);
            if (result == SDMMC_ERR_INITIALIZED)
                result = SDMMC_ERR_NONE;
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

        case SDMMC_NAND_CRYPT_SETUP:
            nandCrypt_Init();
            break;
    }

    return result;
}
