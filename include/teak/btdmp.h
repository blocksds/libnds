// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBTEAK_BTDMP_H__
#define LIBTEAK_BTDMP_H__

#include <teak/types.h>

#define BTDMP_REG_BASE                      0x8280
#define BTDMP_CHANNEL_LEN                   0x80
#define BTDMP_CHANNEL_REG_BASE(x)           (BTDMP_REG_BASE + (x) * BTDMP_CHANNEL_LEN)

#define REG_BTDMP_RECEIVE_ENABLE(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x1E))

#define BTDMP_RECEIVE_ENABLE_OFF            0x0000
#define BTDMP_RECEIVE_ENABLE_ON             0x8000

#define REG_BTDMP_TRANSMIT_UNK20(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x20))
#define REG_BTDMP_TRANSMIT_UNK22(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x22))
#define REG_BTDMP_TRANSMIT_UNK24(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x24))
#define REG_BTDMP_TRANSMIT_UNK26(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x26))
#define REG_BTDMP_TRANSMIT_UNK28(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x28))
#define REG_BTDMP_TRANSMIT_UNK2A(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x2A))

#define REG_BTDMP_TRANSMIT_ENABLE(x)        (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x3E))

#define BTDMP_TRANSMIT_ENABLE_OFF           0x0000
#define BTDMP_TRANSMIT_ENABLE_ON            0x8000

#define REG_BTDMP_TRANSMIT_FIFO_STAT(x)     (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x42))

#define BTDMP_TRANSMIT_FIFO_STAT_FULL       (1 << 3)
#define BTDMP_TRANSMIT_FIFO_STAT_EMPTY      (1 << 4)

#define REG_BTDMP_TRANSMIT_FIFO_DATA(x)     (*(vs16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x46))

#define REG_BTDMP_TRANSMIT_FIFO_CONFIG(x)   (*(vs16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x4A))

#define BTDMP_TRANSMIT_FIFO_CONFIG_FLUSH    (1 << 2)

static inline void btdmpEnableReceive(int channel)
{
    REG_BTDMP_RECEIVE_ENABLE(channel) = BTDMP_RECEIVE_ENABLE_ON;
}

static inline void btdmpEnableTransmit(int channel)
{
    REG_BTDMP_TRANSMIT_ENABLE(channel) = BTDMP_TRANSMIT_ENABLE_ON;
}

static inline void btdmpDisableReceive(int channel)
{
    REG_BTDMP_RECEIVE_ENABLE(channel) = BTDMP_RECEIVE_ENABLE_OFF;
}

static inline void btdmpDisableTransmit(int channel)
{
    REG_BTDMP_TRANSMIT_ENABLE(channel) = BTDMP_TRANSMIT_ENABLE_OFF;
}

static inline void btdmpFlushTransmitFifo(int channel)
{
    REG_BTDMP_TRANSMIT_FIFO_CONFIG(channel) = BTDMP_TRANSMIT_FIFO_CONFIG_FLUSH;
}

#endif // LIBTEAK_BTDMP_H__
