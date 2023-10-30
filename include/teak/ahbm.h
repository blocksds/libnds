// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBTEAK_AHBM_H__
#define LIBTEAK_AHBM_H__

#include <teak/types.h>

// For more information about the meaning of all of the concepts in this file,
// check the document "AMBA AHB Protocol Specification" from ARM: "ARM IHI 0033C
// (ID090921)".

/// AHBM Status (R)
#define REG_AHBM_STATUS                     (*(vu16 *)0x80E0)
// Applications wait for all bits to be 0 before connecting AHBM to DMA.

#define AHBM_STATUS_QUEUE_BUSY              BIT(2) // Inverted HREADY?
#define AHBM_STATUS_ERROR                   BIT(4) // HRESP? 3DS only?

/// AHBM Channel 0..2 Configure Burst/Data (R/W)
#define REG_AHBM_CH_CFG1(n)                 (*(vu16 *)(0x80E2 + (n) * 6))

#define AHBM_CH_CFG1_BURST_SINGLE           (0 << 0) // HBURST[2:0]
#define AHBM_CH_CFG1_BURST_INCR             (1 << 0)
#define AHBM_CH_CFG1_BURST_WRAP4            (2 << 0)
#define AHBM_CH_CFG1_BURST_INCR4            (3 << 0)
#define AHBM_CH_CFG1_BURST_WRAP8            (4 << 0)
#define AHBM_CH_CFG1_BURST_INCR8            (5 << 0)
// - WRAP16 and INCR16 are not supported or so and act like INCR mode.
// - Bit 3 hangs if no burst is used.

#define AHBM_CH_CFG1_SIZE_8BIT              (0 << 4) // HSIZE[2:0]
#define AHBM_CH_CFG1_SIZE_16BIT             (1 << 4)
#define AHBM_CH_CFG1_SIZE_32BIT             (2 << 4)
// - Bit 6 is probably the MSB of the HSIZE signal, but since the bus is only
//   32 bit, that bit is ignored.
// - Bit 7 may be HMASTLOCK, as GBATEK mentions it hangs the transfer.
// - Bits 8-11 may be HPROT, which is most likely unused on the DSi.

/// AHBM Channel 0..2 Configure Whatever (R/W)
#define REG_AHBM_CH_CFG2(n)                 (*(vu16 *)(0x80E4 + (n) * 6))

#define AHBM_CH_CFG2_READ                   (0 << 8) ///< Read external memory
#define AHBM_CH_CFG2_WRITE                  (1 << 8) ///< HWRITE. Write external memory

// Applications always set this to 1 (but also works when 0). It may be HNONSEC,
// which is probably not used in DSi, but still connected.
#define AHBM_CH_CFG2_USUALLY_ONE            BIT(9)

/// AHBM Channel 0..2 Configure DMA (R/W)
#define REG_AHBM_CH_CFG_DMA(n)              (*(vu16 *)(0x80E6 + (n) * 6))

#define AHBM_CH_CFG_DMA_CONNECT_CH(n)       BIT(n)

/// Checks whether any AHBM channel is busy.
///
/// @return Returns 1 if any AHBM channel is busy, 0 otherwise.
static inline u16 ahbmIsBusy(void)
{
    return (REG_AHBM_STATUS & AHBM_STATUS_QUEUE_BUSY) ? 1 : 0;
}

/// Sets up an AHBM channel.
///
/// @param channel The AHBM channel to setup.
/// @param a First configuration setting.
/// @param b Second configuration setting.
/// @param dma_channel_mask The DMA channels to assign to this AHBM channel.
static inline void ahbmConfigChannel(int channel, u16 a, u16 b, u16 dma_channel_mask)
{
    REG_AHBM_CH_CFG1(channel) = a;
    REG_AHBM_CH_CFG2(channel) = b;
    REG_AHBM_CH_CFG_DMA(channel) = dma_channel_mask;
}

/// Resets an AHBM channel.
///
/// @param channel The AHBM channel to setup.
static inline void ahbmResetChannel(int channel)
{
    REG_AHBM_CH_CFG_DMA(channel) = 0;
}

#endif // LIBTEAK_AHBM_H__
