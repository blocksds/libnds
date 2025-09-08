// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <stdio.h>
#include <string.h>

#include <nds/bios.h>
#include <nds/card.h>
#include <nds/cothread.h>
#include <nds/dma.h>
#include <nds/interrupts.h>
#include <nds/memory.h>

void cardWriteCommand(const u8 *command)
{
    REG_AUXSPICNTH = CARD_SPICNTH_ENABLE | CARD_SPICNTH_IRQ;

    for (int index = 0; index < 8; index++)
        REG_CARD_COMMAND[7 - index] = command[index];
}

void cardPolledTransfer(u32 flags, u32 *destination, u32 length, const u8 *command)
{
    u32 data;
    cardWriteCommand(command);

    REG_ROMCTRL = flags;
    u32 *target = destination + length;

    do
    {
        // Read data if available
        if (REG_ROMCTRL & CARD_DATA_READY)
        {
            data = REG_CARD_DATA_RD;
            if (destination != NULL && destination < target)
                *destination++ = data;
        }
    } while (REG_ROMCTRL & CARD_BUSY);
}

void cardStartTransfer(const u8 *command, u32 *destination, int channel, u32 flags)
{
    cardWriteCommand(command);

    // Set up a DMA channel to transfer a word every time the card makes one
    dmaSetParams(channel, (const void*) &REG_CARD_DATA_RD, destination,
                 DMA_ENABLE | DMA_START_CARD | DMA_32_BIT | DMA_REPEAT
                 | DMA_SRC_FIX | 0x0001);

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

    REG_AUXSPICNTH = CARD_SPICNTH_ENABLE | CARD_SPICNTH_IRQ;
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
    REG_ROMCTRL = CARD_ACTIVATE | CARD_nRESET | CARD_CLK_SLOW | CARD_BLK_SIZE(5)
                  | CARD_DELAY2(0x18);
    u32 read = 0;

    do
    {
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
#define NDS_CARD_BLOCK_ALIGN ((NDS_CARD_BLOCK_SIZE) - 1)
#define NDS_CARD_BLOCK_ALIGN_MASK (~(NDS_CARD_BLOCK_ALIGN))

#define NDS_CARD_PAGE_SIZE 0x1000
#define NDS_CARD_PAGE_ALIGN ((NDS_CARD_PAGE_SIZE) - 1)
#define NDS_CARD_PAGE_ALIGN_MASK (~(NDS_CARD_PAGE_ALIGN))

// The cardRead() code supports a minimum read size of 0x4 (word alignment).
// This should be supported by most emulators, but is not guaranteed to work
// correctly on all hardware in circulation; retail software assumes a minimum
// read size of 0x200, so we stick to it here.
#define NDS_CARD_READ_SIZE NDS_CARD_BLOCK_SIZE
#define NDS_CARD_READ_ALIGN ((NDS_CARD_READ_SIZE) - 1)
#define NDS_CARD_READ_ALIGN_MASK (~(NDS_CARD_READ_ALIGN))

static inline void cardReadInternal(void *dest, size_t offset, size_t len, uint32_t flags)
{
    cardParamCommand(CARD_CMD_DATA_READ, offset, flags | CARD_nRESET | CARD_ACTIVATE,
                     dest, len >> 2);
}

void cardRead(void *dest, size_t offset, size_t len, uint32_t flags)
{
    uint8_t buffer[NDS_CARD_BLOCK_SIZE] __attribute__((aligned(4)));
    uint8_t *pc = dest;

    while (len)
    {
        // are both the read offset and the destination buffer read-aligned?
        while (!(offset & NDS_CARD_READ_ALIGN) && !(((uint32_t) pc) & NDS_CARD_READ_ALIGN) && len >= NDS_CARD_READ_SIZE)
        {
            size_t len_aligned;
            if (NDS_CARD_READ_SIZE == NDS_CARD_BLOCK_SIZE)
            {
                len_aligned = NDS_CARD_BLOCK_SIZE;
            }
            else
            {
                len_aligned = len & NDS_CARD_READ_ALIGN_MASK;
                if (len_aligned > NDS_CARD_BLOCK_SIZE)
                    len_aligned = NDS_CARD_BLOCK_SIZE;
            }

            // check if block is in the same 0x1000 page
            size_t len_masked = ((offset | 0xFFF) + 1) - offset;
            if (len_aligned > len_masked)
                len_aligned = len_masked;

            // fast direct read
            cardReadInternal(pc, offset, len_aligned, flags | CARD_BLK_SIZE(1));

            pc += len_aligned;
            offset += len_aligned;
            len -= len_aligned;

            if (!len)
                break;
        }

        // slow buffered read: approximate to word alignment, then memcpy
        size_t block_offset = (offset & NDS_CARD_READ_ALIGN);
        size_t block_len = len;

        // offset is not word-aligned; adjust offset to have word alignment
        if (block_offset)
        {
            block_len += block_offset;
            offset -= block_offset;
        }

        // adjust block_len to have word alignment
        if (block_len > NDS_CARD_BLOCK_SIZE)
            block_len = NDS_CARD_BLOCK_SIZE;
        // check if block is in the same 0x1000 page
        size_t block_len_masked = ((offset | 0xFFF) + 1) - offset;
        if (block_len > block_len_masked)
            block_len = block_len_masked;
        size_t block_len_aligned = (block_len + NDS_CARD_READ_ALIGN) & NDS_CARD_READ_ALIGN_MASK;

        // the length of data actually written to dest
        size_t dest_block_len = block_len - block_offset;

        cardReadInternal(buffer, offset, block_len_aligned, flags | CARD_BLK_SIZE(1));

        memcpy(pc, buffer + block_offset, dest_block_len);
        offset += block_len;
        pc += dest_block_len;
        len -= dest_block_len;
    }
}
