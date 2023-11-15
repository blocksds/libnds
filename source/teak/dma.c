// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#include <teak/ahbm.h>
#include <teak/dma.h>

// Note 1
// ------
//
// Channel 0 is automatically enabled on power up because it is used for
// ARM-side data transfers via DSP_PDATA (port 4004300h). Apply settings to it
// that seem to prevent lockups of the DMA pipe or so.
//
// Don't disable this channel or use it for copies! If you do, FIFO transfers
// using DSP_PDATA won't work.
//
// Note 2
// ------
//
// According to the document "AMBA AHB Protocol Specification" from ARM: "ARM
// IHI 0033C (ID090921)", transfers can't cross 1KB address boundaries:
//
//     Managers must not attempt to start an incrementing burst that crosses a
//     1KB address boundary. Split it into multiple copies in that case.
//
// This only affects addresses on the ARM9 side, not the DSP side.

#define PAGE_SIZE 1024

void dmaInit(void)
{
    const u16 dma_channel = 0;

    u16 old_sel = REG_DMA_CHANNEL_SEL;

    REG_DMA_CHANNEL_SEL = dma_channel;

    REG_DMA_START &= ~BIT(dma_channel);
    REG_DMA_CH_UNK_81DC &= ~0xF; // ???
    REG_DMA_CH_UNK_81DC |= 0x10; // ???
    REG_DMA_START |= BIT(dma_channel);

    REG_DMA_CHANNEL_SEL = old_sel;
}

static void dmaTransferArm9ToDspUnsafe(u16 dma_channel, u32 src, u16 dst, u16 len)
{
    const u16 ahbm_channel = 2;

    ahbmResetChannel(ahbm_channel);

    REG_DMA_CHANNEL_SEL = dma_channel;

    REG_DMA_CH_SRC_LO = src & 0xFFFF;
    REG_DMA_CH_SRC_HI = src >> 16;

    REG_DMA_CH_DST_LO = dst;
    REG_DMA_CH_DST_HI = 0;

    REG_DMA_CH_DIM0_LEN = len;
    REG_DMA_CH_DIM1_LEN = 1;
    REG_DMA_CH_DIM2_LEN = 1;

    REG_DMA_CH_DIM0_SRC_STEP = 2;
    REG_DMA_CH_DIM1_SRC_STEP = 1;
    REG_DMA_CH_DIM2_SRC_STEP = 1;

    REG_DMA_CH_DIM0_DST_STEP = 1;
    REG_DMA_CH_DIM1_DST_STEP = 1;
    REG_DMA_CH_DIM2_DST_STEP = 1;

    REG_DMA_CH_XFER_CONFIG = DMA_CH_XFER_CONFIG_SRC_ARM_AHBM
                           | DMA_CH_XFER_CONFIG_DST_DSP_DATA
                           | DMA_CH_XFER_CONFIG_RW_SIMULTANEOUS
                           | DMA_CH_XFER_SPEED_SLOWEST;

    REG_DMA_CH_UNK_81DC = 0x300; // ???

    ahbmConfigChannel(ahbm_channel,
                      AHBM_CH_CFG1_SIZE_16BIT | AHBM_CH_CFG1_BURST_INCR,
                      AHBM_CH_CFG2_READ | AHBM_CH_CFG2_USUALLY_ONE,
                      AHBM_CH_CFG_DMA_CONNECT_CH(dma_channel));

    REG_DMA_CH_CONTROL = DMA_CH_CONTROL_START;

    while ((REG_DMA_DIM2_END & BIT(dma_channel)) == 0);

    ahbmResetChannel(ahbm_channel);
}

s16 dmaTransferArm9ToDsp(u16 dma_channel, u32 src, void *dst, u16 len)
{
    // It isn't safe to start a copy of size 0, it will still copy one word
    if (len == 0)
        return -1;

    // This function can't copy more than 1 KB of data
    if (len > (PAGE_SIZE / 2))
        return -2;

    // Protect channel used for FIFO transfers
    if (dma_channel == 0)
        return -3;

    const u16 page_mask = ~(PAGE_SIZE - 1);

    // This function can't copy more than one page in one call, so it is okay to
    // clamp the ARM9 address to 16 bit.
    u16 end = src + (len << 1) - 1;

    u16 page_base = src & page_mask;
    u16 page_end = end & page_mask;

    if (page_base != page_end)
        return -4;

    // TODO: Is this needed? What if there are two AHBM channels set up at the
    // same time?
    while (ahbmIsBusy());

    dmaTransferArm9ToDspUnsafe(dma_channel, src, (u16)dst, len);
    return 0;
}

static void dmaTransferDspToArm9Unsafe(u16 dma_channel, const void *src, u32 dst, u16 len)
{
    // AHBM channel to be used for the transfer
    const u16 ahbm_channel = 2;

    ahbmResetChannel(ahbm_channel);

    REG_DMA_CHANNEL_SEL = dma_channel;

    REG_DMA_CH_SRC_LO = (u16)src;
    REG_DMA_CH_SRC_HI = 0x0000;

    REG_DMA_CH_DST_LO = dst & 0xFFFF;
    REG_DMA_CH_DST_HI = dst >> 16;

    REG_DMA_CH_DIM0_LEN = len;
    REG_DMA_CH_DIM1_LEN = 1;
    REG_DMA_CH_DIM2_LEN = 1;

    REG_DMA_CH_DIM0_SRC_STEP = 1;
    REG_DMA_CH_DIM1_SRC_STEP = 1;
    REG_DMA_CH_DIM2_SRC_STEP = 1;

    REG_DMA_CH_DIM0_DST_STEP = 2;
    REG_DMA_CH_DIM1_DST_STEP = 1;
    REG_DMA_CH_DIM2_DST_STEP = 1;

    REG_DMA_CH_XFER_CONFIG = DMA_CH_XFER_CONFIG_SRC_DSP_DATA
                           | DMA_CH_XFER_CONFIG_DST_ARM_AHBM
                           | DMA_CH_XFER_CONFIG_RW_SIMULTANEOUS
                           | DMA_CH_XFER_SPEED_SLOWEST;

    REG_DMA_CH_UNK_81DC = 0x300; // ???

    ahbmConfigChannel(ahbm_channel,
                      AHBM_CH_CFG1_SIZE_16BIT | AHBM_CH_CFG1_BURST_INCR,
                      AHBM_CH_CFG2_WRITE | AHBM_CH_CFG2_USUALLY_ONE,
                      AHBM_CH_CFG_DMA_CONNECT_CH(dma_channel));

    REG_DMA_CH_CONTROL = DMA_CH_CONTROL_START;

    while ((REG_DMA_DIM2_END & BIT(dma_channel)) == 0);

    ahbmResetChannel(ahbm_channel);
}

s16 dmaTransferDspToArm9(u16 dma_channel, const void *src, u32 dst, u16 len)
{
    // It isn't safe to start a copy of size 0, it will still copy one word
    if (len == 0)
        return -1;

    // This function can't copy more than 1 KB of data
    if (len > (PAGE_SIZE / 2))
        return -2;

    // Protect channel used for FIFO transfers
    if (dma_channel == 0)
        return -3;

    const u16 page_mask = ~(PAGE_SIZE - 1);

    // This function can't copy more than one page in one call, so it is okay to
    // clamp the ARM9 address to 16 bit.
    u16 end = dst + (len << 1) - 1;

    u16 page_base = dst & page_mask;
    u16 page_end = end & page_mask;

    if (page_base != page_end)
        return -4;

    // TODO: Is this needed? What if there are two AHBM channels set up at the
    // same time?
    while (ahbmIsBusy());

    dmaTransferDspToArm9Unsafe(dma_channel, src, dst, len);
    return 0;
}

static void dmaTransferArm9ToDspAsyncUnsafe(u16 dma_channel, u32 src, void *dst, u16 len)
{
    // AHBM channel to be used for the transfer
    const u16 ahbm_channel = 1;

    // Disable AHBM
    ahbmResetChannel(ahbm_channel);

    // Select the DMA channel that we are going to setup
    REG_DMA_CHANNEL_SEL = dma_channel;

    REG_DMA_START &= ~(1 << dma_channel); // Set dma channel for config

    REG_DMA_CH_SRC_LO = src & 0xFFFF;
    REG_DMA_CH_SRC_HI = src >> 16;

    REG_DMA_CH_DST_LO = (u16)dst;
    REG_DMA_CH_DST_HI = 0;

    REG_DMA_CH_DIM0_LEN = len;
    REG_DMA_CH_DIM1_LEN = 1;
    REG_DMA_CH_DIM2_LEN = 1;

    REG_DMA_CH_DIM0_SRC_STEP = 4; // Read 4 bytes
    REG_DMA_CH_DIM0_DST_STEP = 2; // Write 2 words (4 bytes)
    REG_DMA_CH_DIM1_SRC_STEP = 1;
    REG_DMA_CH_DIM1_DST_STEP = 1;
    REG_DMA_CH_DIM2_SRC_STEP = 1;
    REG_DMA_CH_DIM2_DST_STEP = 1;

    REG_DMA_CH_XFER_CONFIG = DMA_CH_XFER_CONFIG_SRC_ARM_AHBM
                           | DMA_CH_XFER_CONFIG_DST_DSP_DATA
                           | DMA_CH_XFER_CONFIG_RW_SIMULTANEOUS
                           | DMA_CH_XFER_CONFIG_32BIT
                           | DMA_CH_XFER_SPEED_SLOWEST;

    REG_DMA_CH_UNK_81DC = 0x300; // ???

    REG_DMA_CH_CONTROL = DMA_CH_CONTROL_START;

    // Setup AHBM to read the ARM9 memory and connect it to DMA channel 5

#if 0
    u16 config = AHBM_CH_CFG1_SIZE_32BIT;

    // Important: Bursts with defined length may never cross 1024 byte boundaries!
    if ((src & ~0x3FF) != ((src + (len << 1)) & ~0x3FF))
        config |= AHBM_CH_CFG1_BURST_INCR;
    else
        config |= AHBM_CH_CFG1_BURST_INCR4;
#else
    u16 config = AHBM_CH_CFG1_SIZE_16BIT | AHBM_CH_CFG1_BURST_INCR;
#endif

    ahbmConfigChannel(ahbm_channel, config,
                      AHBM_CH_CFG2_READ | AHBM_CH_CFG2_USUALLY_ONE,
                      AHBM_CH_CFG_DMA_CONNECT_CH(dma_channel));

    REG_DMA_START |= BIT(dma_channel);
}

s16 dmaTransferArm9ToDspAsync(u16 dma_channel, u32 src, void *dst, u16 len)
{
    // It isn't safe to start a copy of size 0, it will still copy one word
    if (len == 0)
        return -1;

    // This function can't copy more than 1 KB of data
    if (len > (PAGE_SIZE / 2))
        return -2;

    // Protect channel used for FIFO transfers
    if (dma_channel == 0)
        return -3;

    const u16 page_mask = ~(PAGE_SIZE - 1);

    // This function can't copy more than one page in one call, so it is okay to
    // clamp the ARM9 address to 16 bit.
    u16 end = src + (len << 1) - 1;

    u16 page_base = src & page_mask;
    u16 page_end = end & page_mask;

    if (page_base != page_end)
        return -4;

    // TODO: Is this needed? What if there are two AHBM channels set up at the
    // same time?
    while (ahbmIsBusy());

    dmaTransferArm9ToDspAsyncUnsafe(dma_channel, src, dst, len);
    return 0;
}
