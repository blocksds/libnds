// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/arm9/paddle.h
///
/// @brief Paddle device slot-2 addon support.

#ifndef LIBNDS_NDS_ARM9_PADDLE_H__
#define LIBNDS_NDS_ARM9_PADDLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <nds/ndstypes.h>

/// Check for the paddle
///
/// @return Returns true if a paddle is in the slot-2.
bool paddleIsInserted(void);

/// Obtain the current paddle state.
///
/// Returns a u16 containing a 12bit number (fixed point fraction), incrementing
/// for clockwise rotations and decrementing for counterclockwise/
///
/// @return The value.
u16 paddleRead(void);

/// Resets the paddle device.
///
/// May change the current value to 0xFFF, 0x000, or 0x001. May perform other
/// unknown internal reset operations. Normally not needed.
void paddleReset(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_PADDLE_H__
