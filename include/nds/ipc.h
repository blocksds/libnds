// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Inter Processor Communication

#ifndef LIBNDS_NDS_IPC_H__
#define LIBNDS_NDS_IPC_H__

#include <nds/ndstypes.h>

// Synchronization register
#define REG_IPC_SYNC (*(vuint16 *)0x04000180)

enum IPC_SYNC_BITS {
    IPC_SYNC_IRQ_ENABLE  = BIT(14),
    IPC_SYNC_IRQ_REQUEST = BIT(13)
};

static inline void IPC_SendSync(unsigned int sync)
{
    REG_IPC_SYNC = (REG_IPC_SYNC & 0xf0ff) | (((sync) & 0x0f) << 8)
                   | IPC_SYNC_IRQ_REQUEST;
}

static inline int IPC_GetSync(void)
{
    return REG_IPC_SYNC & 0x0f;
}

// FIFO
#define REG_IPC_FIFO_TX (*(vu32 *)0x4000188)
#define REG_IPC_FIFO_RX (*(vu32 *)0x4100000)
#define REG_IPC_FIFO_CR (*(vu16 *)0x4000184)

enum IPC_CONTROL_BITS {
    IPC_FIFO_SEND_EMPTY = (1 << 0),
    IPC_FIFO_SEND_FULL  = (1 << 1),
    IPC_FIFO_SEND_IRQ   = (1 << 2),
    IPC_FIFO_SEND_CLEAR = (1 << 3),
    IPC_FIFO_RECV_EMPTY = (1 << 8),
    IPC_FIFO_RECV_FULL  = (1 << 9),
    IPC_FIFO_RECV_IRQ   = (1 << 10),
    IPC_FIFO_ERROR      = (1 << 14),
    IPC_FIFO_ENABLE     = (1 << 15)
};

#endif // LIBNDS_NDS_IPC_H__
