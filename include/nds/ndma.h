// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (c) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_NDMA_H__
#define LIBNDS_NDS_NDMA_H__

/// @file nds/ndma.h
///
/// @brief NDMA helpers.

#include <nds/ndstypes.h>

#define NDMA_GCR            (*(vuint32 *)0x04004100)

#define NDMA_GCR_DELAY_SCALER(n)    ((n) << 16)
#define NDMA_GCR_ROUND_ROBIN        BIT(31)

#define NDMA0_SRC           (*(vuint32 *)0x04004104)
#define NDMA0_DEST          (*(vuint32 *)0x04004108)
#define NDMA0_LENGTH        (*(vuint32 *)0x0400410C)
#define NDMA0_BLENGTH       (*(vuint32 *)0x04004110)
#define NDMA0_BDELAY        (*(vuint32 *)0x04004114)
#define NDMA0_FILL          (*(vuint32 *)0x04004118)
#define NDMA0_CR            (*(vuint32 *)0x0400411C)

#define NDMA1_SRC           (*(vuint32 *)0x04004120)
#define NDMA1_DEST          (*(vuint32 *)0x04004124)
#define NDMA1_LENGTH        (*(vuint32 *)0x04004128)
#define NDMA1_BLENGTH       (*(vuint32 *)0x0400412C)
#define NDMA1_BDELAY        (*(vuint32 *)0x04004130)
#define NDMA1_FILL          (*(vuint32 *)0x04004134)
#define NDMA1_CR            (*(vuint32 *)0x04004138)

#define NDMA2_SRC           (*(vuint32 *)0x0400413C)
#define NDMA2_DEST          (*(vuint32 *)0x04004140)
#define NDMA2_LENGTH        (*(vuint32 *)0x04004144)
#define NDMA2_BLENGTH       (*(vuint32 *)0x04004148)
#define NDMA2_BDELAY        (*(vuint32 *)0x0400414C)
#define NDMA2_FILL          (*(vuint32 *)0x04004150)
#define NDMA2_CR            (*(vuint32 *)0x04004154)

#define NDMA3_SRC           (*(vuint32 *)0x04004158)
#define NDMA3_DEST          (*(vuint32 *)0x0400415C)
#define NDMA3_LENGTH        (*(vuint32 *)0x04004160)
#define NDMA3_BLENGTH       (*(vuint32 *)0x04004164)
#define NDMA3_BDELAY        (*(vuint32 *)0x04004168)
#define NDMA3_FILL          (*(vuint32 *)0x0400416C)
#define NDMA3_CR            (*(vuint32 *)0x04004170)

#define NDMA_SRC(n)         (*(vuint32 *)(0x04004104 + ((n) * 0x1C)))
#define NDMA_DEST(n)        (*(vuint32 *)(0x04004108 + ((n) * 0x1C)))
#define NDMA_LENGTH(n)      (*(vuint32 *)(0x0400410C + ((n) * 0x1C)))
#define NDMA_BLENGTH(n)     (*(vuint32 *)(0x04004110 + ((n) * 0x1C)))
#define NDMA_BDELAY(n)      (*(vuint32 *)(0x04004114 + ((n) * 0x1C)))
#define NDMA_FILL(n)        (*(vuint32 *)(0x04004118 + ((n) * 0x1C)))
#define NDMA_CR(n)          (*(vuint32 *)(0x0400411C + ((n) * 0x1C)))

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

/// Determines if the specified NDMA channel is busy.
///
/// @param channel the NDMA channel to check (0 - 3).
/// @return non zero if busy, 0 if channel is free.
static inline int ndmaBusy(uint8_t channel)
{
    return (NDMA_CR(channel) & NDMA_BUSY) >> 31;
}

#endif // LIBNDS_NDS_NDMA_H__
