// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_ARM9_TEAK_DSP_H__
#define LIBNDS_NDS_ARM9_TEAK_DSP_H__

#include <nds/ndstypes.h>

/// @file nds/arm9/teak/dsp.h
///
/// @brief DSP general utilities.
///
/// This file contains general definitions and helpers to use the DSP of the
/// DSi.

#define DSP_MEM_ADDR_TO_DSP(addr)       ((u16)(((u32)(addr)) >> 1))
#define DSP_MEM_ADDR_TO_CPU(addr)       ((u32)((addr) << 1))

#define DSP_MEM_32BIT_TO_DSP(x)         (u32)(((u32)(x) >> 16) | ((u32)(x) << 16))

/// DSP Transfer Data Read FIFO (R/W)
#define REG_DSP_PDATA           (*(vu16 *)(0x04004300))

/// DSP Transfer Address (W)
#define REG_DSP_PADR            (*(vu16 *)(0x04004304))

/// DSP Configuration (R/W)
#define REG_DSP_PCFG            (*(vu16 *)0x4004308)

#define DSP_PCFG_RESET          1

#define DSP_PCFG_AUTOINC        (1 << 1)

typedef enum
{
    DSP_PCFG_RLEN_1 = 0,
    DSP_PCFG_RLEN_8 = 1,
    DSP_PCFG_RLEN_16 = 2,
    DSP_PCFG_RLEN_FREE = 3
} DSP_PCFG_RLEN;

#define DSP_PCFG_RLEN_SHIFT     2
#define DSP_PCFG_RLEN_MASK      (3 << DSP_PCFG_RLEN_SHIFT)
#define DSP_PCFG_RLEN(x)        ((x) << DSP_PCFG_RLEN_SHIFT)

#define DSP_PCFG_RSTART         (1 << 4)

#define DSP_PCFG_IE_REP_SHIFT   9

#define DSP_PCFG_IE_REP0        (1 << DSP_PCFG_IE_REP_SHIFT)
#define DSP_PCFG_IE_REP1        (1 << (DSP_PCFG_IE_REP_SHIFT + 1))
#define DSP_PCFG_IE_REP2        (1 << (DSP_PCFG_IE_REP_SHIFT + 2))

typedef enum
{
    DSP_PCFG_MEMSEL_DATA = 0,
    DSP_PCFG_MEMSEL_MMIO = 1,
    DSP_PCFG_MEMSEL_PROG = 5
} DSP_PCFG_MEMSEL;

#define DSP_PCFG_MEMSEL_SHIFT   12
#define DSP_PCFG_MEMSEL_MASK    (0xF << DSP_PCFG_MEMSEL_SHIFT)
#define DSP_PCFG_MEMSEL(x)      ((x) << DSP_PCFG_MEMSEL_SHIFT)

/// DSP Status (R)
#define REG_DSP_PSTS            (*(vu16 *)0x400430C)

#define DSP_PSTS_RD_XFER_BUSY   (1 << 0)
#define DSP_PSTS_WR_XFER_BUSY   (1 << 1)
#define DSP_PSTS_PERI_RESET     (1 << 2)
#define DSP_PSTS_RD_FIFO_FULL   (1 << 5)
#define DSP_PSTS_RD_FIFO_READY  (1 << 6)
#define DSP_PSTS_WR_FIFO_FULL   (1 << 7)
#define DSP_PSTS_WR_FIFO_EMPTY  (1 << 8)

#define DSP_PSTS_REP_NEW_SHIFT  10

#define DSP_PSTS_REP0_NEW       (1 << DSP_PSTS_REP_NEW_SHIFT)
#define DSP_PSTS_REP1_NEW       (1 << (DSP_PSTS_REP_NEW_SHIFT + 1))
#define DSP_PSTS_REP2_NEW       (1 << (DSP_PSTS_REP_NEW_SHIFT + 2))

#define DSP_PSTS_CMD_UNREAD_SHIFT 13

#define DSP_PSTS_CMD0_UNREAD    (1 << DSP_PSTS_CMD_UNREAD_SHIFT)
#define DSP_PSTS_CMD1_UNREAD    (1 << (DSP_PSTS_CMD_UNREAD_SHIFT + 1))
#define DSP_PSTS_CMD2_UNREAD    (1 << (DSP_PSTS_CMD_UNREAD_SHIFT + 2))

/// ARM9-to-DSP Semaphore (R/W)
#define REG_DSP_PSEM            (*(vu16 *)0x4004310)
/// DSP-to-ARM9 Semaphore Mask (R/W)
#define REG_DSP_PMASK           (*(vu16 *)0x4004314)
/// DSP-to-ARM9 Semaphore Clear (W)
#define REG_DSP_PCLEAR          (*(vu16 *)0x4004318)
/// DSP-to-ARM9 Semaphore Data (R)
#define REG_DSP_SEM             (*(vu16 *)0x400431C)

/// DSP Command Register 0 (R/W) (ARM9 to DSP)
#define REG_DSP_CMD0            (*(vu16 *)0x4004320)
/// DSP Reply Register 0 (R) (DSP to ARM9)
#define REG_DSP_REP0            (*(vu16 *)0x4004324)

/// DSP Command Register 1 (R/W) (ARM9 to DSP)
#define REG_DSP_CMD1            (*(vu16 *)0x4004328)
/// DSP Reply Register 1 (R) (DSP to ARM9)
#define REG_DSP_REP1            (*(vu16 *)0x400432C)

/// DSP Command Register 2 (R/W) (ARM9 to DSP)
#define REG_DSP_CMD2            (*(vu16 *)0x4004330)
/// DSP Reply Register 2 (R) (DSP to ARM9)
#define REG_DSP_REP2            (*(vu16 *)0x4004334)

/// Function that executes a delay of a few cycles.
void dspSpinWait(void);

void dspSetBlockReset(bool reset);
void dspSetClockEnabled(bool enabled);
void dspResetInterface(void);
void dspSetCoreResetOn(void);
void dspSetCoreResetOff(u16 repIrqMask);
void dspPowerOn(void);
void dspPowerOff(void);

/// This powers on the DSP, loads a TLF file and executes it.
///
/// The user must allocate NWRAM to the ARM9 before calling this function by
/// using nwramSetBlockMapping(). Remember that the default MPU setup only
/// allows mapping it to the range 0x03000000 - 0x03800000.
///
/// @param tlf Pointer to the TLF data in RAM.
/// @return true on success.
bool dspExecuteTLF(const void *tlf);

/// Sends data using one of the CMD registers.
///
/// This function waits until the previous value has been read by the DSP.
///
/// @param id ID of the CMD register (0 to 2).
/// @param data Data to send.
void dspSendData(int id, u16 data);

/// Checks if a CMD register is available to receive new data.
///
/// @param id ID of the CMD register (0 to 2).
/// @return true if it is available.
bool dspSendDataReady(int id);

/// Receives data from one of the REP registers.
///
/// This function waits until there is a value to be read.
///
/// @param id ID of the REP register (0 to 2).
/// @return Received data.
u16 dspReceiveData(int id);

/// Checks if a REP register has any data available.
///
/// @param id ID of the REP register (0 to 2).
/// @return true if there is data to be read.
bool dspReceiveDataReady(int id);

static inline void dspSetSemaphore(u16 mask)
{
    REG_DSP_PSEM = mask;
}

static inline void dspSetSemaphoreMask(u16 mask)
{
    REG_DSP_PMASK = mask;
}

static inline void dspClearSemaphore(u16 mask)
{
    REG_DSP_PCLEAR = mask;
}

static inline u16 dspGetSemaphore(void)
{
    dspSpinWait();
    return REG_DSP_SEM;
}

#endif // LIBNDS_NDS_ARM9_TEAK_DSP_H__
