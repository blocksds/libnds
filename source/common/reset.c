// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2010 Dave Murphy (WinterMute)
// Copyright (c) 2023 Antonio Niño Díaz

#include <nds/ipc.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

#include "fifo_ipc_messages.h"

static void resetSync(void)
{
    while ((REG_IPC_SYNC & 0x0f) != 1);
    REG_IPC_SYNC = 0x100;
    while ((REG_IPC_SYNC & 0x0f) != 0);
    REG_IPC_SYNC = 0;
}

#ifdef ARM7
void resetARM9(u32 address)
{
    *((vu32 *)0x02FFFE24) = address;

    REG_IPC_FIFO_TX = FIFO_ADDRESSBIT | FIFO_IMMEDIATEBIT
                    | FIFO_ARM7_REQUESTS_ARM9_RESET;
    resetSync();
}
#else
void resetARM7(u32 address)
{
    *((vu32 *)0x02FFFE34) = address;

    REG_IPC_FIFO_TX = FIFO_ADDRESSBIT | FIFO_IMMEDIATEBIT
                    | FIFO_ARM9_REQUESTS_ARM7_RESET;
    resetSync();
}
#endif
