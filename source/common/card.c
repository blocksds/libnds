// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (Dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <stdio.h>

#include <nds/bios.h>
#include <nds/card.h>
#include <nds/dma.h>
#include <nds/memory.h>

void cardWriteCommand(const u8 *command)
{
    REG_AUXSPICNTH = CARD_CR1_ENABLE | CARD_CR1_IRQ;

    for (int index = 0; index < 8; index++)
        REG_CARD_COMMAND[7 - index] = command[index];
}

void cardPolledTransfer(u32 flags, u32 *destination, u32 length, const u8 *command)
{
    u32 data;
    cardWriteCommand(command);

    REG_ROMCTRL = flags;
    u32 *target = destination + length;

    do {
        // Read data if available
        if (REG_ROMCTRL & CARD_DATA_READY)
        {
            data=REG_CARD_DATA_RD;
            if (destination != NULL && destination < target)
                *destination++ = data;
        }
    } while (REG_ROMCTRL & CARD_BUSY);
}

void cardStartTransfer(const u8 *command, u32 *destination, int channel, u32 flags)
{
    cardWriteCommand(command);

    // Set up a DMA channel to transfer a word every time the card makes one
    DMA_SRC(channel) = (u32)&REG_CARD_DATA_RD;
    DMA_DEST(channel) = (u32)destination;
    DMA_CR(channel) = DMA_ENABLE | DMA_START_CARD | DMA_32_BIT | DMA_REPEAT
                    | DMA_SRC_FIX | 0x0001;

    REG_ROMCTRL = flags;
}

u32 cardWriteAndRead(const u8 *command, u32 flags)
{
    cardWriteCommand(command);

    REG_ROMCTRL = flags | CARD_ACTIVATE | CARD_nRESET | CARD_BLK_SIZE(7);

    while (!(REG_ROMCTRL & CARD_DATA_READY));

    return REG_CARD_DATA_RD;
}

void cardParamCommand(u8 command, u32 parameter, u32 flags, u32 *destination, u32 length)
{
    u8 cmdData[8];

    cmdData[7] = (u8)command;
    cmdData[6] = (u8)(parameter >> 24);
    cmdData[5] = (u8)(parameter >> 16);
    cmdData[4] = (u8)(parameter >> 8);
    cmdData[3] = (u8)(parameter >> 0);
    cmdData[2] = 0;
    cmdData[1] = 0;
    cmdData[0] = 0;

    cardPolledTransfer(flags, destination, length, cmdData);
}

void cardReadHeader(u8 *header)
{
    REG_ROMCTRL = 0;
    REG_AUXSPICNTH = 0;

    swiDelay(167550);

    REG_AUXSPICNTH = CARD_CR1_ENABLE | CARD_CR1_IRQ;
    REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED;

    while (REG_ROMCTRL & CARD_BUSY);

    cardReset();

    while (REG_ROMCTRL & CARD_BUSY);

    uint32_t flags = CARD_ACTIVATE | CARD_nRESET | CARD_CLK_SLOW
                   | CARD_BLK_SIZE(1) | CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F);

    cardParamCommand(CARD_CMD_HEADER_READ, 0, flags, (u32 *)header, 512 / 4);
}

u32 cardReadID(u32 flags)
{
    const u8 command[8] = {0, 0, 0, 0, 0, 0, 0, CARD_CMD_HEADER_CHIPID};

    return cardWriteAndRead(command, flags);
}

void cardReset(void)
{
    const u8 cmdData[8] = { 0, 0, 0, 0, 0, 0, 0, CARD_CMD_DUMMY };

    cardWriteCommand(cmdData);
    REG_ROMCTRL=CARD_ACTIVATE|CARD_nRESET|CARD_CLK_SLOW|CARD_BLK_SIZE(5)|CARD_DELAY2(0x18);
    u32 read=0;

    do {
        if (REG_ROMCTRL & CARD_DATA_READY)
        {
            if (read < 0x2000)
            {
                u32 data = REG_CARD_DATA_RD;
                (void)data;
                read += 4;
            }
        }
    } while (REG_ROMCTRL & CARD_BUSY);
}

#define NDS_CARD_BLOCK_SIZE 0x200 // CARD_BLK_SIZE(1)

// Size must be smaller or equal than NDS_CARD_BLOCK_SIZE
static void cardReadBlock(void *dest, size_t offset, size_t size)
{
    const uint32_t flags =
        CARD_DELAY1(0x1FFF) | CARD_DELAY2(0x3F) | CARD_CLK_SLOW |
        CARD_nRESET | CARD_SEC_CMD | CARD_SEC_DAT | CARD_ACTIVATE |
        CARD_BLK_SIZE(1);

    cardParamCommand(CARD_CMD_DATA_READ, offset, flags, dest, size);
}

// The destination and size must be word-aligned
void cardRead(void *dest, size_t offset, size_t size)
{
    char *curr_dest = dest;

    while (size > 0)
    {
        // The cardReadBlock() function can only read up to NDS_CARD_BLOCK_SIZE
        size_t curr_size = size;
        if (curr_size > NDS_CARD_BLOCK_SIZE)
            curr_size = NDS_CARD_BLOCK_SIZE;

        cardReadBlock(curr_dest, offset, curr_size);
        curr_dest += curr_size;
        offset += curr_size;
        size -= curr_size;
    }
}
