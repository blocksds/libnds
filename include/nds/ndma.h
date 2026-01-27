// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_NDMA_H__
#define LIBNDS_NDS_NDMA_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/ndma.h
///
/// @brief NDMA helpers.

#include <nds/ndstypes.h>

#define NDMA_MAX_CHANNELS   (4)

#define REG_NDMA_GCR        (*(vuint32 *)0x04004100)

#define NDMA_GCR_DELAY_SCALER(n)    ((n) << 16)
#define NDMA_GCR_ROUND_ROBIN        (1u << 31)
#define NDMA_GCR_FIXED_METHOD       (0u << 31)

#define REG_NDMA0_SRC       (*(vuint32 *)0x04004104)
#define REG_NDMA0_DEST      (*(vuint32 *)0x04004108)
#define REG_NDMA0_LENGTH    (*(vuint32 *)0x0400410C)
#define REG_NDMA0_BLENGTH   (*(vuint32 *)0x04004110)
#define REG_NDMA0_BDELAY    (*(vuint32 *)0x04004114)
#define REG_NDMA0_FILL      (*(vuint32 *)0x04004118)
#define REG_NDMA0_CR        (*(vuint32 *)0x0400411C)

#define REG_NDMA1_SRC       (*(vuint32 *)0x04004120)
#define REG_NDMA1_DEST      (*(vuint32 *)0x04004124)
#define REG_NDMA1_LENGTH    (*(vuint32 *)0x04004128)
#define REG_NDMA1_BLENGTH   (*(vuint32 *)0x0400412C)
#define REG_NDMA1_BDELAY    (*(vuint32 *)0x04004130)
#define REG_NDMA1_FILL      (*(vuint32 *)0x04004134)
#define REG_NDMA1_CR        (*(vuint32 *)0x04004138)

#define REG_NDMA2_SRC       (*(vuint32 *)0x0400413C)
#define REG_NDMA2_DEST      (*(vuint32 *)0x04004140)
#define REG_NDMA2_LENGTH    (*(vuint32 *)0x04004144)
#define REG_NDMA2_BLENGTH   (*(vuint32 *)0x04004148)
#define REG_NDMA2_BDELAY    (*(vuint32 *)0x0400414C)
#define REG_NDMA2_FILL      (*(vuint32 *)0x04004150)
#define REG_NDMA2_CR        (*(vuint32 *)0x04004154)

#define REG_NDMA3_SRC       (*(vuint32 *)0x04004158)
#define REG_NDMA3_DEST      (*(vuint32 *)0x0400415C)
#define REG_NDMA3_LENGTH    (*(vuint32 *)0x04004160)
#define REG_NDMA3_BLENGTH   (*(vuint32 *)0x04004164)
#define REG_NDMA3_BDELAY    (*(vuint32 *)0x04004168)
#define REG_NDMA3_FILL      (*(vuint32 *)0x0400416C)
#define REG_NDMA3_CR        (*(vuint32 *)0x04004170)

#define REG_NDMA_SRC(n)     (*(vuint32 *)(0x04004104 + ((n) * 0x1C)))
#define REG_NDMA_DEST(n)    (*(vuint32 *)(0x04004108 + ((n) * 0x1C)))
#define REG_NDMA_LENGTH(n)  (*(vuint32 *)(0x0400410C + ((n) * 0x1C)))
#define REG_NDMA_BLENGTH(n) (*(vuint32 *)(0x04004110 + ((n) * 0x1C)))
#define REG_NDMA_BDELAY(n)  (*(vuint32 *)(0x04004114 + ((n) * 0x1C)))
#define REG_NDMA_FILL(n)    (*(vuint32 *)(0x04004118 + ((n) * 0x1C)))
#define REG_NDMA_CR(n)      (*(vuint32 *)(0x0400411C + ((n) * 0x1C)))

#define NDMA_BDELAY_CYCLES(n)   (n)
#define NDMA_BDELAY_DIV_1       (0)
#define NDMA_BDELAY_DIV_4       (1 << 16)
#define NDMA_BDELAY_DIV_16      (2 << 16)
#define NDMA_BDELAY_DIV_64      (3 << 16)

#define NDMA_ENABLE             BIT(31)
#define NDMA_BUSY               BIT(31)
#define NDMA_IRQ_REQ            BIT(30)
#define NDMA_REPEAT             BIT(29)

#define NDMA_BLOCK_SCALER(n)    ((n) << 16)

#define NDMA_SRC_INC            (0)
#define NDMA_SRC_DEC            (1 << 13)
#define NDMA_SRC_FIX            (2 << 13)
#define NDMA_SRC_FILL           (3 << 13)

#define NDMA_DST_INC            (0)
#define NDMA_DST_DEC            (1 << 10)
#define NDMA_DST_FIX            (2 << 10)

#define NDMA_START_NOW          (1 << 28)
#define NDMA_START_TIMER0       (0)
#define NDMA_START_TIMER1       (1 << 24)
#define NDMA_START_TIMER2       (2 << 24)
#define NDMA_START_TIMER3       (3 << 24)
#define NDMA_START_TIMER(n)     ((n) << 24)
#define NDMA_START_CARD         (4 << 24)
#define NDMA_START_VBL          (6 << 24)

#ifdef ARM9
#define NDMA_START_HBL          (7 << 24)
#define NDMA_START_LINE         (8 << 24)
// TODO: NDMA mode 0x09?
#define NDMA_START_FIFO         (10 << 24)
#define NDMA_START_CAMERA       (11 << 24)
#endif

#ifdef ARM7
#define NDMA_START_NTR_WIFI     (7 << 24)
#define NDMA_START_SDMMC        (8 << 24)
#define NDMA_START_TWL_WIFI     (9 << 24)
#define NDMA_START_AES_IN       (10 << 24)
#define NDMA_START_AES_OUT      (11 << 24)
#define NDMA_START_MIC          (12 << 24)
#endif

// Outdated defines for backwards compatibility

#define NDMA_GCR            REG_NDMA_GCR

#define NDMA0_SRC           REG_NDMA0_SRC
#define NDMA0_DEST          REG_NDMA0_DEST
#define NDMA0_LENGTH        REG_NDMA0_LENGTH
#define NDMA0_BLENGTH       REG_NDMA0_BLENGTH
#define NDMA0_BDELAY        REG_NDMA0_BDELAY
#define NDMA0_FILL          REG_NDMA0_FILL
#define NDMA0_CR            REG_NDMA0_CR

#define NDMA1_SRC           REG_NDMA1_SRC
#define NDMA1_DEST          REG_NDMA1_DEST
#define NDMA1_LENGTH        REG_NDMA1_LENGTH
#define NDMA1_BLENGTH       REG_NDMA1_BLENGTH
#define NDMA1_BDELAY        REG_NDMA1_BDELAY
#define NDMA1_FILL          REG_NDMA1_FILL
#define NDMA1_CR            REG_NDMA1_CR

#define NDMA2_SRC           REG_NDMA2_SRC
#define NDMA2_DEST          REG_NDMA2_DEST
#define NDMA2_LENGTH        REG_NDMA2_LENGTH
#define NDMA2_BLENGTH       REG_NDMA2_BLENGTH
#define NDMA2_BDELAY        REG_NDMA2_BDELAY
#define NDMA2_FILL          REG_NDMA2_FILL
#define NDMA2_CR            REG_NDMA2_CR

#define NDMA3_SRC           REG_NDMA3_SRC
#define NDMA3_DEST          REG_NDMA3_DEST
#define NDMA3_LENGTH        REG_NDMA3_LENGTH
#define NDMA3_BLENGTH       REG_NDMA3_BLENGTH
#define NDMA3_BDELAY        REG_NDMA3_BDELAY
#define NDMA3_FILL          REG_NDMA3_FILL
#define NDMA3_CR            REG_NDMA3_CR

#define NDMA_SRC(n)         REG_NDMA_SRC(n)
#define NDMA_DEST(n)        REG_NDMA_DEST(n)
#define NDMA_LENGTH(n)      REG_NDMA_LENGTH(n)
#define NDMA_BLENGTH(n)     REG_NDMA_BLENGTH(n)
#define NDMA_BDELAY(n)      REG_NDMA_BDELAY(n)
#define NDMA_FILL(n)        REG_NDMA_FILL(n)
#define NDMA_CR(n)          REG_NDMA_CR(n)

/// Determines if the specified NDMA channel is busy.
///
/// @param channel
///     The NDMA channel to check (0 - 3).
/// @return
///     Non zero if busy, 0 if channel is free.
static inline int ndmaBusy(uint8_t channel)
{
    return (NDMA_CR(channel) & NDMA_BUSY) >> 31;
}

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_NDMA_H__
