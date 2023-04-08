// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2010 Dave Murphy (WinterMute)

#include <nds/ndstypes.h>
#include <nds/ipc.h>
#include <nds/system.h>

#ifdef ARM7

void resetARM9(u32 address) {
	*((vu32*)0x02FFFE24) = address;

#else

void resetARM7(u32 address) {
	*((vu32*)0x02FFFE34) = address;

#endif

	REG_IPC_FIFO_TX = 0x0c04000c;
	while((REG_IPC_SYNC & 0x0f) != 1);
	REG_IPC_SYNC = 0x100;
	while((REG_IPC_SYNC & 0x0f) != 0);
	REG_IPC_SYNC = 0;
}

