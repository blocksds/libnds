// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <nds/arm9/cache.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/memory.h>

// Function to ask the ARM7 to read from the slot-1 using card commands
bool cardReadArm7(void *dest, size_t offset, size_t size)
{
	DC_FlushRange(dest, size);

	FifoMessage msg;
	msg.type = SLOT1_CARD_READ;
	msg.sdParams.startsector = offset;
	msg.sdParams.numsectors = size;
	msg.sdParams.buffer = dest;

	// Let the ARM7 access the slot-1
	sysSetCardOwner(BUS_OWNER_ARM7);

	fifoSendDatamsg(FIFO_STORAGE, sizeof(msg), (u8 *)&msg);

	fifoWaitValue32(FIFO_STORAGE);
	DC_InvalidateRange(dest, size);

	int result = fifoGetValue32(FIFO_STORAGE);
	return result != 0;
}
