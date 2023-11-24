// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBTEAK_BTDMP_H__
#define LIBTEAK_BTDMP_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file teak/btdmp.h
///
/// @brief BTDMP (speakers/microphone FIFO).

#include <teak/types.h>

#define BTDMP_REG_BASE                      0x8280
#define BTDMP_CHANNEL_LEN                   0x80
#define BTDMP_CHANNEL_REG_BASE(x)           (BTDMP_REG_BASE + (x) * BTDMP_CHANNEL_LEN)

#define REG_BTDMP_RECEIVE_UNK00(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x00))
#define REG_BTDMP_RECEIVE_UNK02(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x02))
#define REG_BTDMP_RECEIVE_UNK04(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x04))
#define REG_BTDMP_RECEIVE_UNK06(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x06))
#define REG_BTDMP_RECEIVE_UNK08(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x08))
#define REG_BTDMP_RECEIVE_UNK0A(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x0A))
#define REG_BTDMP_RECEIVE_UNK0C(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x0C))
#define REG_BTDMP_RECEIVE_UNK0E(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x0E))
#define REG_BTDMP_RECEIVE_UNK10(x)          (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x10))

#define REG_BTDMP_RECEIVE_ENABLE(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x1E))

#define BTDMP_RECEIVE_ENABLE_OFF            0x0000
#define BTDMP_RECEIVE_ENABLE_ON             0x8000

#define REG_BTDMP_TRANSMIT_UNK20(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x20))
#define REG_BTDMP_TRANSMIT_UNK22(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x22))
#define REG_BTDMP_TRANSMIT_UNK24(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x24))
#define REG_BTDMP_TRANSMIT_UNK26(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x26))
#define REG_BTDMP_TRANSMIT_UNK28(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x28))
#define REG_BTDMP_TRANSMIT_UNK2A(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x2A))
#define REG_BTDMP_TRANSMIT_UNK2C(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x2C))
#define REG_BTDMP_TRANSMIT_UNK2E(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x2E))
#define REG_BTDMP_TRANSMIT_UNK30(x)         (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x30))

#define REG_BTDMP_TRANSMIT_ENABLE(x)        (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x3E))

#define BTDMP_TRANSMIT_ENABLE_OFF           0x0000
#define BTDMP_TRANSMIT_ENABLE_ON            0x8000

#define REG_BTDMP_RECEIVE_FIFO_STAT(x)      (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x40))

#define BTDMP_RECEIVE_FIFO_STAT_FULL        (1 << 3)
#define BTDMP_RECEIVE_FIFO_STAT_EMPTY       (1 << 4)

#define REG_BTDMP_TRANSMIT_FIFO_STAT(x)     (*(vu16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x42))

#define BTDMP_TRANSMIT_FIFO_STAT_FULL       (1 << 3)
#define BTDMP_TRANSMIT_FIFO_STAT_EMPTY      (1 << 4)

#define REG_BTDMP_RECEIVE_FIFO_DATA(x)      (*(vs16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x44))
#define REG_BTDMP_TRANSMIT_FIFO_DATA(x)     (*(vs16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x46))

#define REG_BTDMP_RECEIVE_FIFO_CONFIG(x)    (*(vs16*)(BTDMP_CHANNEL_REG_BASE(x) + 0x48))

#define BTDMP_RECEIVE_FIFO_CONFIG_FLUSH     (1 << 2)

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

static inline void btdmpFlushReceiveFifo(int channel)
{
    REG_BTDMP_RECEIVE_FIFO_CONFIG(channel) = BTDMP_RECEIVE_FIFO_CONFIG_FLUSH;
}

/// Setups a BTDMP channel to output audio to the DS speakers.
///
/// Note: Remember to setup REG_SNDEXTCNT from the ARM7 to enable sound output
/// from the DSP. For example, for 50% DSP output and 50% ARM7 output:
/// ```
/// REG_SNDEXTCNT = SNDEXTCNT_ENABLE | SNDEXTCNT_FREQ_32KHZ | SNDEXTCNT_RATIO(4);
/// ```
///
/// @param channel The BTDMP channel to use.
/// @param irq_index The CPU interrupt to use (0 to 2).
void btdmpSetupOutputSpeakers(int channel, int irq_index);

/// Checks if the transmit FIFO of a BTDMP channel is full or not.
///
/// @param channel The BTDMP channel to check.
/// @return 1 if the FIFO is full, 0 otherwise.
static inline int btdmpTransmitFifoFull(int channel)
{
    if (REG_BTDMP_TRANSMIT_FIFO_STAT(channel) & BTDMP_TRANSMIT_FIFO_STAT_FULL)
        return 1;

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif // LIBTEAK_BTDMP_H__
