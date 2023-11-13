// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2014 Dave Murphy (WinterMute)

#include <string.h>

#include <nds/arm7/serial.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/interrupts.h>
#include <nds/system.h>

static u8 readwriteSPI(u8 data)
{
    REG_SPIDATA = data;
    SerialWaitBusy();
    return REG_SPIDATA;
}

void readFirmware(u32 address, void *destination, u32 size)
{
    int oldIME = enterCriticalSection();
    u8 *buffer = destination;

    // Read command
    REG_SPICNT = SPI_ENABLE | SPI_BYTE_MODE | SPI_CONTINUOUS | SPI_DEVICE_FIRMWARE;
    readwriteSPI(FIRMWARE_READ);

    // Set the address
    readwriteSPI((address >> 16) & 0xFF);
    readwriteSPI((address >> 8) & 0xFF);
    readwriteSPI(address & 0xFF);

    // Read the data
    for (u32 i = 0; i < size; i++)
        buffer[i] = readwriteSPI(0);

    REG_SPICNT = 0;
    leaveCriticalSection(oldIME);
}

int readJEDEC(u8 *destination, u32 size)
{
    if (destination == NULL)
        return -1;

    // JEDEC is always 3 bytes.
    if (size < 3)
        return -2;

    int oldIME = enterCriticalSection();
    REG_SPICNT = SPI_ENABLE | SPI_BYTE_MODE | SPI_CONTINUOUS | SPI_DEVICE_FIRMWARE;
    readwriteSPI(FIRMWARE_RDID); // get JEDEC
    for (int i = 0; i < 3; i++)
        destination[i] = readwriteSPI(0);

    REG_SPICNT = 0;
    leaveCriticalSection(oldIME);

    return 0;
}

static int writeFirmwarePage(u32 address, u8 *buffer)
{
    u8 pagebuffer[256];
    readFirmware(address, pagebuffer, 256);

    if (memcmp(pagebuffer, buffer, 256) == 0)
        return 0;

    int oldIME = enterCriticalSection();

    // Write enable
    REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
    readwriteSPI(FIRMWARE_WREN);
    REG_SPICNT = 0;

    // Wait for Write Enable Latch to be set
    REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
    readwriteSPI(FIRMWARE_RDSR);
    while ((readwriteSPI(0) & 0x02) == 0); // Write Enable Latch
    REG_SPICNT = 0;

    // Page write
    REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
    readwriteSPI(FIRMWARE_PW);

    // Set the address
    readwriteSPI((address >> 16) & 0xFF);
    readwriteSPI((address >> 8) & 0xFF);
    readwriteSPI(address & 0xFF);

    for (int i = 0; i < 256; i++)
        readwriteSPI(buffer[i]);

    REG_SPICNT = 0;

    // Wait for programming to finish
    REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
    readwriteSPI(FIRMWARE_RDSR);
    while (readwriteSPI(0) & 0x01); // Write In Progress
    REG_SPICNT = 0;

    leaveCriticalSection(oldIME);

    // Read it back and verify
    readFirmware(address, pagebuffer, 256);
    if (memcmp(pagebuffer, buffer, 256) == 0)
        return 0;
    return -1;
}

int writeFirmware(u32 address, void *source, u32 size)
{
    if (((address & 0xff) != 0) || ((size & 0xff) != 0))
        return -1;

    u8 *buffer = source;
    int response = -1;

    while (size > 0)
    {
        size -= 256;
        if (writeFirmwarePage(address + size, buffer + size))
            break;
    }

    if (size == 0)
        response = 0;

    return response;
}

void firmwareMsgHandler(int bytes, void *user_data)
{
    (void)user_data;

    FifoMessage msg;

    int response = -1;

    fifoGetDatamsg(FIFO_FIRMWARE, bytes, (u8*)&msg);

    switch (msg.type)
    {
        case FW_READ:
            readFirmware(msg.blockParams.address, msg.blockParams.buffer,
                         msg.blockParams.length);
            response = 0;
            break;
        case FW_WRITE:
            response = writeFirmware(msg.blockParams.address, msg.blockParams.buffer,
                                     msg.blockParams.length);
            break;
    }

    fifoSendValue32(FIFO_FIRMWARE, response);
}
