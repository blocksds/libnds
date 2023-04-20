// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file dma.h
///
/// @brief Wrapper functions for direct memory access hardware.
///
/// <div class="fileHeader">
/// The DS has 4 hardware direct memory access devices per CPU which can be used
/// to transfer or fill chunks of memeory without CPU intervention. Using DMA is
/// generaly faster than CPU copies (memcpy, swiCopy, for loops, etc..).
///
/// DMA has no access to data caches on the DS and as such will give unexpected
/// results when DMAing data from main memory. The cache must be flushed as
/// follows when using DMA to ensure proper opertion on the ARM9:
///
/// <pre>
/// DC_FlushRange(source, sizeof(dataToCopy));
/// dmaCopy(source, destination, sizeof(dataToCopy));
/// </pre>
///
/// </div>

#ifndef LIBNDS_NDS_DMA_H__
#define LIBNDS_NDS_DMA_H__

#include <nds/ndstypes.h>

#define DMA0_SRC        (*(vuint32 *)0x040000B0)
#define DMA0_DEST       (*(vuint32 *)0x040000B4)
#define DMA0_CR         (*(vuint32 *)0x040000B8)

#define DMA1_SRC        (*(vuint32 *)0x040000BC)
#define DMA1_DEST       (*(vuint32 *)0x040000C0)
#define DMA1_CR         (*(vuint32 *)0x040000C4)

#define DMA2_SRC        (*(vuint32 *)0x040000C8)
#define DMA2_DEST       (*(vuint32 *)0x040000CC)
#define DMA2_CR         (*(vuint32 *)0x040000D0)

#define DMA3_SRC        (*(vuint32 *)0x040000D4)
#define DMA3_DEST       (*(vuint32 *)0x040000D8)
#define DMA3_CR         (*(vuint32 *)0x040000DC)

#define DMA_SRC(n)      (*(vuint32 *)(0x040000B0 + ((n) * 12)))
#define DMA_DEST(n)     (*(vuint32 *)(0x040000B4 + ((n) * 12)))
#define DMA_CR(n)       (*(vuint32 *)(0x040000B8 + ((n) * 12)))

#ifdef ARM9
#    define DMA_FILL(n) (*(vuint32 *)(0x040000E0 + ((n) * 4)))
#endif

// DMA control register contents.
// The defaults are 16-bit, increment source/dest addresses, no IRQ.
#define DMA_ENABLE          BIT(31)
#define DMA_BUSY            BIT(31)
#define DMA_IRQ_REQ         BIT(30)

#define DMA_START_NOW       0
#define DMA_START_CARD      (5 << 27)

#ifdef ARM7
#    define DMA_START_VBL   BIT(27)
#endif

#ifdef ARM9
#    define DMA_START_HBL   BIT(28)
#    define DMA_START_VBL   BIT(27)
#    define DMA_START_FIFO  (7 << 27)
#    define DMA_DISP_FIFO   (4 << 27)
#endif

#define DMA_16_BIT          0
#define DMA_32_BIT          BIT(26)

#define DMA_REPEAT          BIT(25)

#define DMA_SRC_INC         (0)
#define DMA_SRC_DEC         BIT(23)
#define DMA_SRC_FIX         BIT(24)

#define DMA_DST_INC         (0)
#define DMA_DST_DEC         BIT(21)
#define DMA_DST_FIX         BIT(22)
#define DMA_DST_RESET       (3 << 21)

#define DMA_COPY_WORDS      (DMA_ENABLE | DMA_32_BIT | DMA_START_NOW)
#define DMA_COPY_HALFWORDS  (DMA_ENABLE | DMA_16_BIT | DMA_START_NOW)
#define DMA_FIFO            (DMA_ENABLE | DMA_32_BIT | DMA_DST_FIX | DMA_START_FIFO)

/// Copies from source to destination on one of the 4 available channels in
/// words.
///
/// @param channel The DMA channel to use (0 - 3).
/// @param src The source to copy from.
/// @param dest The destination to copy to.
/// @param size The size in bytes of the data to copy. Will be truncated to the
///             nearest word (4 bytes).
static inline void dmaCopyWords(uint8_t channel, const void *src, void *dest,
                                uint32_t size)
{
    DMA_SRC(channel) = (uint32_t)src;
    DMA_DEST(channel) = (uint32_t)dest;
    DMA_CR(channel) = DMA_COPY_WORDS | (size >> 2);
    while (DMA_CR(channel) & DMA_BUSY)
        ;
}

/// Copies from source to destination on one of the 4 available channels in half
/// words.
///
/// @param channel The DMA channel to use (0 - 3).
/// @param src The source to copy from.
/// @param dest The destination to copy to
/// @param size The size in bytes of the data to copy. Will be truncated to the
///             nearest half word (2 bytes)
static inline void dmaCopyHalfWords(uint8_t channel, const void *src, void *dest,
                                    uint32_t size)
{
    DMA_SRC(channel) = (uint32_t)src;
    DMA_DEST(channel) = (uint32_t)dest;
    DMA_CR(channel) = DMA_COPY_HALFWORDS | (size >> 1);
    while (DMA_CR(channel) & DMA_BUSY)
        ;
}

/// Copies from source to destination using channel 3 of DMA available channels
/// in half words.
///
/// @param source The source to copy from.
/// @param dest The destination to copy to
/// @param size The size in bytes of the data to copy.  Will be truncated to the
///             nearest half word (2 bytes).
static inline void dmaCopy(const void *source, void *dest, uint32_t size)
{
    DMA_SRC(3) = (uint32_t)source;
    DMA_DEST(3) = (uint32_t)dest;
    DMA_CR(3) = DMA_COPY_HALFWORDS | (size >> 1);
    while (DMA_CR(3) & DMA_BUSY)
        ;
}

/// Copies from source to destination on one of the 4 available channels in
/// half words.
///
/// This function returns immediately after starting the transfer.
///
/// @param channel The DMA channel to use (0 - 3).
/// @param src The source to copy from.
/// @param dest The destination to copy to.
/// @param size The size in bytes of the data to copy. Will be truncated to the
///             nearest word (4 bytes)
static inline void dmaCopyWordsAsynch(uint8_t channel, const void *src, void *dest,
                                      uint32_t size)
{
    DMA_SRC(channel) = (uint32_t)src;
    DMA_DEST(channel) = (uint32_t)dest;
    DMA_CR(channel) = DMA_COPY_WORDS | (size >> 2);
}

/// Copies from source to destination on one of the 4 available channels in half
/// words.
///
/// This function returns immediately after starting the transfer.
///
/// @param channel The DMA channel to use (0 - 3).
/// @param src The source to copy from.
/// @param dest The destination to copy to.
/// @param size The size in bytes of the data to copy. Will be truncated to the
///             nearest half word (2 bytes)
static inline void dmaCopyHalfWordsAsynch(uint8_t channel, const void *src,
                                          void *dest, uint32_t size)
{
    DMA_SRC(channel) = (uint32_t)src;
    DMA_DEST(channel) = (uint32_t)dest;
    DMA_CR(channel) = DMA_COPY_HALFWORDS | (size >> 1);
}

/// Copies from source to destination using channel 3 of DMA available channels
/// in half words.
///
/// This function returns immediately after starting the transfer.
///
/// @param src The source to copy from.
/// @param dest The destination to copy to.
/// @param size The size in bytes of the data to copy. Will be truncated to the
///             nearest half word (2 bytes)
static inline void dmaCopyAsynch(const void *source, void *dest, uint32_t size)
{
    DMA_SRC(3) = (uint32_t)source;
    DMA_DEST(3) = (uint32_t)dest;
    DMA_CR(3) = DMA_COPY_HALFWORDS | (size >> 1);
}

/// Fills the source with the supplied value using DMA channel 3.
///
/// @param value The 32 byte value to fill memory with.
/// @param dest The destination to copy to.
/// @param size The size in bytes of the area to fill. Will be truncated to the
///             nearest word (4 bytes).
static inline void dmaFillWords(u32 value, void *dest, uint32_t size)
{
#ifdef ARM7
    (*(vu32 *)0x027FFE04) = value;
    DMA_SRC(3) = 0x027FFE04;
#else
    DMA_FILL(3) = value;
    DMA_SRC(3) = (uint32_t)&DMA_FILL(3);
#endif

    DMA_DEST(3) = (uint32_t)dest;
    DMA_CR(3) = DMA_SRC_FIX | DMA_COPY_WORDS | (size >> 2);
    while (DMA_CR(3) & DMA_BUSY)
        ;
}

/// Fills the source with the supplied value using DMA channel 3.
///
/// @param value The 16 byte value to fill memory with.
/// @param dest The destination to copy to.
/// @param size The size in bytes of the area to fill. Will be truncated to the
///             nearest half word (2 bytes).
static inline void dmaFillHalfWords(u16 value, void *dest, uint32_t size)
{
#ifdef ARM7
    (*(vu32 *)0x027FFE04) = (u32)value;
    DMA_SRC(3) = 0x027FFE04;
#else
    DMA_FILL(3) = (uint32_t)value;
    DMA_SRC(3) = (uint32_t)&DMA_FILL(3);
#endif

    DMA_DEST(3) = (uint32_t)dest;
    DMA_CR(3) = DMA_SRC_FIX | DMA_COPY_HALFWORDS | (size >> 1);
    while (DMA_CR(3) & DMA_BUSY)
        ;
}

/// Determines if the specified channel is busy
//
/// @param channel The DMA channel to check (0 - 3).
/// @return Non zero if busy, 0 if channel is free.
static inline int dmaBusy(uint8_t channel)
{
    return (DMA_CR(channel) & DMA_BUSY) >> 31;
}

#endif // LIBNDS_NDS_DMA_H__
