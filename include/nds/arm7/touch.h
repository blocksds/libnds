// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_ARM7_TOUCH_H__
#define LIBNDS_NDS_ARM7_TOUCH_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM7
#error Touch screen is only available on the ARM7
#endif

/// @file nds/arm7/touch.h
///
/// @brief High-level touchscreen functions for ARM7

#include <nds/arm7/serial.h>
#include <nds/touch.h>

/// Initialize the touch subsystem (NDS/DSi).
void touchInit(void);

/// Apply calibration to raw X/Y touch screen measurements.
///
/// @param rawx
///     Raw X value
/// @param rawy
///     Raw Y value
/// @param px
///     Calibrated X value
/// @param py
///     Calibrated Y value
void touchApplyCalibration(u16 rawx, u16 rawy, u16 *px, u16 *py);

// Do not modify the memory layout; touchReadData() functions rely on it.
typedef struct
{
    u16 rawX[5];
    u16 rawY[5];
    u16 z1[5];
    u16 z2[5];
} touchRawArray;

/// Read a complete, raw touch measurement into the provided buffer.
///
/// @param data
///     Struct to hold the read data.
///
/// @return
///     True if the read is successful, false otherwise.
bool touchReadData(touchRawArray *data);

/// Read a touch X/Y position into the provided buffer.
///
/// @param touchPos
///     Struct to hold the read data.
void touchReadXY(touchPosition *touchPos);

/// Checks if the screen is currently being touched.
///
/// @return
///     If the pen is down it returns true.
bool touchPenDown(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_TOUCH_H__
