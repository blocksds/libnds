// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2024 Antonio Niño Díaz

#include <nds/arm9/cache.h>
#include <nds/arm9/sassert.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/memory.h>

// Function to ask the ARM7 to read from the slot-1 using card commands
bool cardReadArm7(void *dest, size_t offset, size_t size, uint32_t flags)
{
    DC_FlushRange(dest, size);

    FifoMessage msg;
    msg.type = SLOT1_CARD_READ;
    msg.cardParams.offset = offset;
    msg.cardParams.size = size;
    msg.cardParams.buffer = dest;
    msg.cardParams.flags = flags;

    fifoMutexAcquire(FIFO_STORAGE);

    // Let the ARM7 access the slot-1
    sysSetCardOwner(BUS_OWNER_ARM7);

    fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32Async(FIFO_STORAGE);
    int result = fifoGetValue32(FIFO_STORAGE);

    fifoMutexRelease(FIFO_STORAGE);

    return result != 0;
}
