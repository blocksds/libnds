// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef __INPUT_H__
#define __INPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

/**
 * @brief Set the amount of frames the lid has to be closed for to trigger sleep.
 *
 * Setting this value to 0 will suppress system sleep on lid closing.
 *
 * @Param frames The number of frames.
 */
void inputSetLidSleepDuration(u16 frames);

/**
 * @brief Send ARM7-side input information (X, Y, touch, lid) to ARM9 via FIFO.
 *
 * This should ideally be called once per frame on the ARM7 CPU.
 */
void inputGetAndSend(void);

#ifdef __cplusplus
}
#endif

#endif
