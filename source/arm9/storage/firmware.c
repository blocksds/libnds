// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2014 Dave Murphy (WinterMute)
// Copyright (c) 2025 Antonio Niño Díaz

#include <stdlib.h>
#include <string.h>

#include <nds/arm9/cache.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/system.h>

static int readFirmwareInternal(u32 address, void *buffer, u32 length)
{
    FifoMessage msg;

    msg.type = FW_READ;
    msg.blockParams.address = address;
    msg.blockParams.buffer = buffer;
    msg.blockParams.length = length;

    fifoMutexAcquire(FIFO_FIRMWARE);

    fifoSendDatamsg(FIFO_FIRMWARE, sizeof(msg), (u8 *)&msg);

    if (REG_IME == 0)
        fifoWaitValue32(FIFO_FIRMWARE);
    else
        fifoWaitValue32Async(FIFO_FIRMWARE);

    int ret = fifoGetValue32(FIFO_FIRMWARE);

    fifoMutexRelease(FIFO_FIRMWARE);

    DC_InvalidateRange(buffer, length);

    return ret;
}

int readFirmware(u32 address, void *buffer, u32 length)
{
    // The ARM7 can only save the results to main RAM
    if (memBufferIsInMainRam(buffer, length))
    {
        return readFirmwareInternal(address, buffer, length);
    }
    else
    {
        void *temp = malloc(length);
        if (temp == NULL)
            return -1;

        int ret = readFirmwareInternal(address, temp, length);
        // Copy the buffer from main RAM to the destination
        memcpy(buffer, temp, length);

        free(temp);

        return ret;
    }
}

static int writeFirmwareInternal(u32 address, void *buffer, u32 length)
{
    if (((address & 0xff) != 0) || ((length & 0xff) != 0))
        return -1;

    DC_FlushRange(buffer, length);

    FifoMessage msg;

    msg.type = FW_WRITE;
    msg.blockParams.address = address;
    msg.blockParams.buffer = buffer;
    msg.blockParams.length = length;

    fifoMutexAcquire(FIFO_FIRMWARE);

    fifoSendDatamsg(FIFO_FIRMWARE, sizeof(msg), (u8 *)&msg);

    if (REG_IME == 0)
        fifoWaitValue32(FIFO_FIRMWARE);
    else
        fifoWaitValue32Async(FIFO_FIRMWARE);

    int ret = fifoGetValue32(FIFO_FIRMWARE);

    fifoMutexRelease(FIFO_FIRMWARE);

    return ret;
}

int writeFirmware(u32 address, void *buffer, u32 length)
{
    // The ARM7 can only read the data from main RAM
    if (memBufferIsInMainRam(buffer, length))
    {
        return writeFirmwareInternal(address, buffer, length);
    }
    else
    {
        void *temp = malloc(length);
        if (temp == NULL)
            return -1;

        // Copy buffer to main RAM from the source
        memcpy(temp, buffer, length);
        int ret = writeFirmwareInternal(address, temp, length);

        free(temp);

        return ret;
    }
}
