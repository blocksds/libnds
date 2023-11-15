// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_TOUCH_H__
#define LIBNDS_NDS_TOUCH_H__

#include <nds/ndstypes.h>

/// @file nds/touch.h
///
/// @brief Contains a struct with touch screen data.

/// Holds data related to the touch screen.
typedef struct touchPosition {
    u16 rawx; ///< Raw x value from the A2D
    u16 rawy; ///< Raw y value from the A2D
    u16 px;   ///< Processes pixel X value
    u16 py;   ///< Processes pixel Y value
    u16 z1;   ///< Raw cross panel resistance
    u16 z2;   ///< Raw cross panel resistance
} touchPosition;

#endif // LIBNDS_NDS_TOUCH_H__
