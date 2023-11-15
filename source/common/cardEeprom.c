// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2010 Michael Noland (joat)
// Copyright (C) 2005-2010 Jason Rogers (Dovoto)
// Copyright (C) 2005-2010 Dave Murphy (WinterMute)

#include <nds/card.h>
#include <nds/dma.h>
#include <nds/memory.h>

u8 cardEepromCommand(u8 command)
{
    u8 retval;

    REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;

    REG_AUXSPIDATA = command;

    eepromWaitBusy();

    REG_AUXSPIDATA = 0;
    eepromWaitBusy();
    retval = REG_AUXSPIDATA;
    REG_AUXSPICNT = /*MODE*/ 0x40;
    return retval;
}

u32 cardEepromReadID(void)
{
    REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;

    REG_AUXSPIDATA = SPI_EEPROM_RDID;

    eepromWaitBusy();
    u32 id = 0;

    for (int i = 0; i < 3; i++)
    {
        REG_AUXSPIDATA = 0;
        eepromWaitBusy();
        id = (id << 8) | REG_AUXSPIDATA;
    }

    REG_AUXSPICNT = /*MODE*/ 0x40;

    return id;
}

int cardEepromGetType(void)
{
    int sr = cardEepromCommand(SPI_EEPROM_RDSR);
    int id = cardEepromReadID();

    if (( sr == 0xff && id == 0xffffff) || (sr == 0 && id == 0))
        return -1;
    if (sr == 0xf0 && id == 0xffffff)
        return 1;
    if (sr == 0x00 && id == 0xffffff)
        return 2;
    if (id != 0xffffff || ( sr == 0x02 && id == 0xffffff))
        return 3;

    return 0;
}

uint32_t cardEepromGetSize(void)
{
    int type = cardEepromGetType();

    if (type == -1)
        return 0;

    if (type == 0)
        return 8192;

    if (type == 1)
        return 512;

    if (type == 2)
    {
        u32 buf1, buf2, buf3 = 0x54534554; // "TEST"

        // Save the first word of the EEPROM
        cardReadEeprom(0, (u8 *)&buf1, 4, type);

        // Write "TEST" to it
        cardWriteEeprom(0, (u8 *)&buf3, 4, type);

        // Loop until the EEPROM mirrors and the first word shows up again
        int size = 8192;
        while (size <= 0x800000)
        {
            cardReadEeprom(size, (u8 *)&buf2, 4, type);
            // Check if it matches, if so check again with another value to
            // ensure no false positives
            if (buf2 == buf3)
            {
                u32 buf4 = 0x74736574; // "test"

                // Write "test" to the first word
                cardWriteEeprom(0, (u8 *)&buf4, 4, type);

                // Check if it still matches
                cardReadEeprom(size, (u8 *)&buf2, 4, type);

                if (buf2 == buf4)
                    break;

                // False match, write "TEST" back and keep going
                cardWriteEeprom(0, (u8 *)&buf3, 4, type);
            }
            size <<= 1;
        }

        // Restore the first word
        cardWriteEeprom(0, (u8 *)&buf1, 4, type);

        return size;
    }

    int device;

    if (type == 3)
    {
        int id = cardEepromReadID();

        device = id & 0xffff;

        if (((id >> 16) & 0xff) == 0x20) // ST
        {
            switch (device)
            {
                case 0x4014:
                    return 1024 * 1024; // 8Mbit (1 meg)
                    break;
                case 0x4013:
                case 0x8013:            // M25PE40
                    return 512 * 1024;  // 4Mbit (512KByte)
                    break;
                case 0x2017:
                    return 8 * 1024 * 1024; // 64Mbit (8 meg)
                    break;
            }
        }

        if (((id >> 16) & 0xff) == 0x62) // Sanyo
        {

            if (device == 0x1100)
                return 512 * 1024; // 4Mbit (512KByte)

        }

        if (((id >> 16) & 0xff) == 0xC2) // Macronix
        {
            switch (device)
            {
                case 0x2211:
                    return 128 * 1024; // 1Mbit (128KByte) - MX25L1021E
                    break;
                case 0x2017:
                    return 8 * 1024 * 1024; // 64Mbit (8 meg)
                    break;
            }
        }

        if (id == 0xffffff)
        {
            int sr = cardEepromCommand(SPI_EEPROM_RDSR);
            if (sr == 2)
                return 128*1024; // 1Mbit (128KByte)
        }


        return 256*1024; // 2Mbit (256KByte)
    }

    return 0;
}

void cardReadEeprom(u32 address, u8 *data, u32 length, u32 addrtype)
{
    REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;
    REG_AUXSPIDATA = 0x03 | ((addrtype == 1) ? address >> 8 << 3 : 0);
    eepromWaitBusy();

    if (addrtype == 3)
    {
        REG_AUXSPIDATA = (address >> 16) & 0xFF;
        eepromWaitBusy();
    }

    if (addrtype >= 2)
    {
        REG_AUXSPIDATA = (address >> 8) & 0xFF;
        eepromWaitBusy();
    }


    REG_AUXSPIDATA = (address) & 0xFF;
    eepromWaitBusy();

    while (length > 0)
    {
        REG_AUXSPIDATA = 0;
        eepromWaitBusy();
        *data++ = REG_AUXSPIDATA;
        length--;
    }

    eepromWaitBusy();
    REG_AUXSPICNT = /*MODE*/ 0x40;
}

void cardWriteEeprom(uint32_t address, uint8_t *data, uint32_t length, uint32_t addrtype)
{
    uint32_t address_end = address + length;
    int maxblocks = 32;

    if (addrtype == 1)
        maxblocks = 16;

    if (addrtype == 2)
        maxblocks = 32;

    if (addrtype == 3)
        maxblocks = 256;

    while (address < address_end)
    {
        // Set WEL (Write Enable Latch)
        REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;
        REG_AUXSPIDATA = 0x06; eepromWaitBusy();
        REG_AUXSPICNT = /*MODE*/ 0x40;

        // program maximum of 32 bytes
        REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;

        if (addrtype == 1)
        {
            // WRITE COMMAND 0x02 + A8 << 3
            REG_AUXSPIDATA = 0x02 | (address & BIT(8)) >> (8 - 3);
            eepromWaitBusy();
            REG_AUXSPIDATA = address & 0xFF;
            eepromWaitBusy();
        }
        else if (addrtype == 2)
        {
            REG_AUXSPIDATA = 0x02;
            eepromWaitBusy();
            REG_AUXSPIDATA = address >> 8;
            eepromWaitBusy();
            REG_AUXSPIDATA = address & 0xFF;
            eepromWaitBusy();
        }
        else if (addrtype == 3)
        {
            REG_AUXSPIDATA = 0x02;
            eepromWaitBusy();
            REG_AUXSPIDATA = (address >> 16) & 0xFF;
            eepromWaitBusy();
            REG_AUXSPIDATA = (address >> 8) & 0xFF;
            eepromWaitBusy();
            REG_AUXSPIDATA = address & 0xFF;
            eepromWaitBusy();
        }

        for (int i = 0; address < address_end && i < maxblocks; i++, address++)
        {
            REG_AUXSPIDATA = *data++;
            eepromWaitBusy();
        }
        REG_AUXSPICNT = /*MODE*/ 0x40;

        // Wait programming to finish
        REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;
        REG_AUXSPIDATA = 0x05;
        eepromWaitBusy();
        do {
            REG_AUXSPIDATA = 0;
            eepromWaitBusy();
        } while (REG_AUXSPIDATA & 0x01); // WIP (Write In Progress) ?

        eepromWaitBusy();
        REG_AUXSPICNT = /*MODE*/ 0x40;
    }
}

// Chip Erase : clear FLASH MEMORY (TYPE 3 ONLY)
void cardEepromChipErase(void)
{
    int sz = cardEepromGetSize();

    for (int sector = 0; sector < sz; sector += 0x10000)
        cardEepromSectorErase(sector);
}

// COMMAND Sec.erase 0xD8
void cardEepromSectorErase(uint32_t address)
{
    // set WEL (Write Enable Latch)
    REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;
    REG_AUXSPIDATA = 0x06;
    eepromWaitBusy();

    REG_AUXSPICNT = /*MODE*/ 0x40;

    // SectorErase 0xD8
    REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;
    REG_AUXSPIDATA = 0xD8;
    eepromWaitBusy();
    REG_AUXSPIDATA = (address >> 16) & 0xFF;
    eepromWaitBusy();
    REG_AUXSPIDATA = (address >> 8) & 0xFF;
    eepromWaitBusy();
    REG_AUXSPIDATA = address & 0xFF;
    eepromWaitBusy();

    REG_AUXSPICNT = /*MODE*/ 0x40;

    // wait erase to finish
    REG_AUXSPICNT = /*E*/ 0x8000 | /*SEL*/ 0x2000 | /*MODE*/ 0x40;
    REG_AUXSPIDATA = 0x05;
    eepromWaitBusy();

    do
    {
        REG_AUXSPIDATA = 0;
        eepromWaitBusy();
    } while (REG_AUXSPIDATA & 0x01); // WIP (Write In Progress) ?

    REG_AUXSPICNT = /*MODE*/ 0x40;
}
