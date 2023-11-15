// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_ARM9_TEAK_FIFO_H__
#define LIBNDS_NDS_ARM9_TEAK_FIFO_H__

#include <nds/arm9/teak/dsp.h>

/// @file nds/arm9/teak/fifo.h
///
/// @brief DSP <-> ARM9 FIFO transfer utilities
///
/// This file contains functions to read and write DSP memory from the ARM9.

/// Sends data to the DSP memory using the FIFO.
///
/// This can't write to program memory.
///
/// @param mem Destination memory. DSP_PCFG_MEMSEL_PROG isn't allowed.
/// @param src Source address in the ARM9 memory map.
/// @param fixedSrc True if the source address is fixed.
/// @param dst Destination address in the selected DSP memory (16 bit units).
/// @param fixedDst True if the destination address is fixed
/// @param length Length of the transfer in 16 bit units.
void dspFifoSend(DSP_PCFG_MEMSEL mem, const u16 *src, bool fixedSrc, u16 dst,
                 bool fixedDst, int length);

/// Sends data to the DSP data memory using the FIFO using default settings.
///
/// This can't write to program memory.
///
/// @param src Source address in the ARM9 memory map.
/// @param dst Destination address in the selected DSP memory (16 bit units).
/// @param length Length of the transfer in 16 bit units.
static inline void dspFifoWriteData(const u16 *src, u16 dst, int length)
{
    dspFifoSend(DSP_PCFG_MEMSEL_DATA, src, false, dst, false, length);
}

/// Receives data from DSP memory using the FIFO.
///
/// This can't read from program memory.
///
/// @param mem Source memory. DSP_PCFG_MEMSEL_PROG not allowed.
/// @param src Source address in the selected DSP memory (16 bit units)
/// @param fixedSrc True if the source address is fixed.
/// @param dst Destination address in the ARM9 memory map.
/// @param fixedDst True if the destination address is fixed.
/// @param length Length of the transfer in 16 bit units.
/// @param lengthMode Length mode of the transfer. Usually DSP_PCFG_RLEN_FREE.
void dspFifoRecv(DSP_PCFG_MEMSEL mem, u16 src, bool fixedSrc, u16 *dst,
                 bool fixedDst, int length, DSP_PCFG_RLEN lengthMode);

/// Receives data from DSP data memory using the FIFO using default settings.
///
/// @param src Source address in the selected DSP memory (16 bit units)
/// @param dst Destination address in the ARM9 memory map.
/// @param length Length of the transfer in 16 bit units.
static inline void dspFifoReadData(u16 src, u16 *dst, int length)
{
    dspFifoRecv(DSP_PCFG_MEMSEL_DATA, src, false, dst, false, length,
                DSP_PCFG_RLEN_FREE);
}

#endif // LIBNDS_NDS_ARM9_TEAK_FIFO_H__
