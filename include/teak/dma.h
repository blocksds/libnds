// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBTEAK_DMA_H__
#define LIBTEAK_DMA_H__

/// @file teak/dma.h
///
/// @brief DMA helpers.
///
/// The DMA registers don't seem to be understood well enough, the functions may
/// be unstable.

/// DMA Channel Start Flags (R/W)
#define REG_DMA_START               (*(vu16 *)0x8184)
/// DMA Channel Pause Flags (R/W)
#define REG_DMA_PAUSE               (*(vu16 *)0x8186)

/// DMA Channel End Flags for Size0 (R)
#define REG_DMA_DIM0_END            (*(vu16 *)0x8188)
/// DMA Channel End Flags for Size1 (R)
#define REG_DMA_DIM1_END            (*(vu16 *)0x818A)
/// DMA Channel End Flags for Size2 (R) (all done)
#define REG_DMA_DIM2_END            (*(vu16 *)0x818C)

/// DMA Select Channel (0 to 7) (R/W)
#define REG_DMA_CHANNEL_SEL         (*(vu16 *)0x81BE)

// DMA Channel: Source Address, bit0-15 (R/W)
#define REG_DMA_CH_SRC_LO           (*(vu16 *)0x81C0)
/// DMA Channel: Source Address, bit16-31 (R/W)
#define REG_DMA_CH_SRC_HI           (*(vu16 *)0x81C2)
/// DMA Channel: Destination Address, bit0-15 (R/W)
#define REG_DMA_CH_DST_LO           (*(vu16 *)0x81C4)
/// DMA Channel: Destination Address, bit16-31 (R/W)
#define REG_DMA_CH_DST_HI           (*(vu16 *)0x81C6)

/// DMA Channel: Size0 (inner dimension) (R/W)
#define REG_DMA_CH_DIM0_LEN         (*(vu16 *)0x81C8)
/// DMA Channel: Size1 (middle dimension) (R/W)
#define REG_DMA_CH_DIM1_LEN         (*(vu16 *)0x81CA)
/// DMA Channel: Size2 (outer dimension) (R/W)
#define REG_DMA_CH_DIM2_LEN         (*(vu16 *)0x81CC)

/// DMA Channel: Source Step0 (signed) (R/W)
#define REG_DMA_CH_DIM0_SRC_STEP    (*(vs16 *)0x81CE)
/// DMA Channel: Source Step1 (signed) (R/W)
#define REG_DMA_CH_DIM0_DST_STEP    (*(vs16 *)0x81D0)
/// DMA Channel: Source Step2 (signed) (R/W)
#define REG_DMA_CH_DIM1_SRC_STEP    (*(vs16 *)0x81D2)
/// DMA Channel: Destination Step0 (signed) (R/W)
#define REG_DMA_CH_DIM1_DST_STEP    (*(vs16 *)0x81D4)
/// DMA Channel: Destination Step1 (signed) (R/W)
#define REG_DMA_CH_DIM2_SRC_STEP    (*(vs16 *)0x81D6)
/// DMA Channel: Destination Step2 (signed) (R/W)
#define REG_DMA_CH_DIM2_DST_STEP    (*(vs16 *)0x81D8)

/// DMA Channel: Memory Area Config (R/W)
#define REG_DMA_CH_XFER_CONFIG      (*(vu16 *)0x81DA)

#define DMA_CH_XFER_CONFIG_SRC_DSP_DATA     (0 << 0)
#define DMA_CH_XFER_CONFIG_SRC_DSP_MMIO     (1 << 0)
#define DMA_CH_XFER_CONFIG_SRC_DSP_CODE     (5 << 0)
#define DMA_CH_XFER_CONFIG_SRC_ARM_AHBM     (7 << 0)

#define DMA_CH_XFER_CONFIG_DST_DSP_DATA     (0 << 4)
#define DMA_CH_XFER_CONFIG_DST_DSP_MMIO     (1 << 4)
#define DMA_CH_XFER_CONFIG_DST_DSP_CODE     (5 << 4)
#define DMA_CH_XFER_CONFIG_DST_ARM_AHBM     (7 << 4)

#define DMA_CH_XFER_CONFIG_RW_SIMULTANEOUS  (1 << 9) // For different memory areas
#define DMA_CH_XFER_CONFIG_32BIT            (1 << 10)

#define DMA_CH_XFER_SPEED_SLOWEST           (0 << 12)
#define DMA_CH_XFER_SPEED_SLOW              (1 << 12)
#define DMA_CH_XFER_SPEED_FAST              (2 << 12)
#define DMA_CH_XFER_SPEED_FASTEST           (3 << 12)

/// DMA Channel: Unknown (R/W)
#define REG_DMA_CH_UNK_81DC         (*(vu16 *)0x81DC)

/// DMA Channel: Start/Stop/Control (R/W)
#define REG_DMA_CH_CONTROL          (*(vu16 *)0x81DE)

#define DMA_CH_CONTROL_IRQ_DIM0     BIT(0)
#define DMA_CH_CONTROL_IRQ_DIM1     BIT(1)
#define DMA_CH_CONTROL_IRQ_DIM2     BIT(2)

#define DMA_CH_CONTROL_DIM2_NO_END  BIT(3)

#define DMA_CH_CONTROL_NO_CHANGE    (0 << 14)
#define DMA_CH_CONTROL_START        (1 << 14)
#define DMA_CH_CONTROL_STOP         (2 << 14)

/// Initializes the DMA system.
void dmaInit(void);

/// Transfer data from the ARM9 memory map to the DSP data memory.
///
/// This function uses AHBM channel 2. DMA channel 0 is required to transfer
/// data with the FIFO functions, so it can't be used by this function.
///
/// This function can't copy data crossing a 1KB boundary on the ARM9 side
/// because of limitations of the AHBM bus.
///
/// @param dma_channel The DMA channel to use (1 to 7, 0 isn't allowed).
/// @param src Source address in the ARM9 memory map.
/// @param dst Destination address in the DSP data memory.
/// @param len Length of the copy in DSP words.
/// @return It returns 0 on success, other values on error.
s16 dmaTransferArm9ToDsp(u16 dma_channel, u32 src, void *dst, u16 len);

/// Transfer data from DSP data memory to the ARM9 memory map.
///
/// This function uses AHBM channel 2. DMA channel 0 is required to transfer
/// data with the FIFO functions, so it can't be used by this function.
///
/// This function can't copy data crossing a 1KB boundary on the ARM9 side
/// because of limitations of the AHBM bus.
///
/// @param dma_channel The DMA channel to use (1 to 7, 0 isn't allowed).
/// @param src Source address in the DSP data memory.
/// @param dst Destination address in the ARM9 memory map.
/// @param len Length of the copy in DSP words.
/// @return It returns 0 on success, other values on error.
s16 dmaTransferDspToArm9(u16 dma_channel, const void *src, u32 dst, u16 len);

/// Starts a transfer of data from the ARM9 memory map to the DSP data memory.
///
/// This function uses AHBM channel 2. DMA channel 0 is required to transfer
/// data with the FIFO functions, so it can't be used by this function.
///
/// This function can't copy data crossing a 1KB boundary on the ARM9 side
/// because of limitations of the AHBM bus.
///
/// Use dmaTransferIsRunning() to verify if the transfer has finished or not.
///
/// @param dma_channel The DMA channel to use (1 to 7, 0 isn't allowed).
/// @param src Source address in the ARM9 memory map.
/// @param dst Destination address in the DSP data memory.
/// @param len Length of the copy in DSP words.
/// @return It returns 0 on success, other values on error.
s16 dmaTransferArm9ToDspAsync(u16 dma_channel, u32 src, void *dst, u16 len);

/// Checks whether a DMA channel is active or not.
///
/// @param dma_channel The DMA channel to use (1 to 7, 0 isn't allowed).
/// @return It returns 1 if the DMA channel is busy, 0 if not.
static inline u16 dmaTransferIsRunning(u16 dma_channel)
{
    if (dma_channel == 0)
        return 0;

    return REG_DMA_START & BIT(dma_channel) ? 1 : 0;
}

#endif // LIBTEAK_DMA_H__
