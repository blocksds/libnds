// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2014 Dave Murphy (WinterMute)

#include <nds/arm9/cache.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/system.h>

void readFirmware(u32 address, void *buffer, u32 length)
{
    FifoMessage msg;

    msg.type = FW_READ;
    msg.blockParams.address = address;
    msg.blockParams.buffer = buffer;
    msg.blockParams.length = length;

    fifoSendDatamsg(FIFO_FIRMWARE, sizeof(msg), (u8 *)&msg);

    while (!fifoCheckValue32(FIFO_FIRMWARE))
        ;
    fifoGetValue32(FIFO_FIRMWARE);
    DC_InvalidateRange(buffer, length);
}

int writeFirmware(u32 address, void *buffer, u32 length)
{
    if (((address & 0xff) != 0) || ((length & 0xff) != 0))
        return -1;
    DC_FlushRange(buffer, length);

    FifoMessage msg;

    msg.type = FW_WRITE;
    msg.blockParams.address = address;
    msg.blockParams.buffer = buffer;
    msg.blockParams.length = length;

    fifoSendDatamsg(FIFO_FIRMWARE, sizeof(msg), (u8 *)&msg);

    while (!fifoCheckValue32(FIFO_FIRMWARE))
        ;

    return (int)fifoGetValue32(FIFO_FIRMWARE);
}
