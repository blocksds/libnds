// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_ARM7_INPUT_H__
#define LIBNDS_NDS_ARM7_INPUT_H__

/// @file nds/arm7/input.h
///
/// @brief Keypad and touch pad ARM7 helpers.

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

/// Set the amount of frames the lid has to be closed for to trigger sleep.
///
/// Setting this value to 0 will suppress system sleep on lid closing.
///
/// @param frames The number of frames.
void inputSetLidSleepDuration(u16 frames);

/// Send ARM7-side input information (X, Y, touch, lid) to ARM9 via FIFO.
///
/// This should ideally be called once per frame on the ARM7 CPU.
void inputGetAndSend(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_INPUT_H__
