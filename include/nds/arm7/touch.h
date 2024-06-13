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

/**
 * @brief Read temperature from the NDS-mode TSC.
 *
 * Note that this does not work on DSi/3DS consoles.
 * It is also not very inaccurate - calibration is required
 * to get a more accurate reading.
 * 
 * @param t1 First measurement output.
 * @param t2 Second measurement output.
 * @return u32 Approximate temperature, in Kelvins.
 */
u32 touchReadTemperature(int *t1, int *t2);

/**
 * @brief Initialize the touch subsystem (NDS/DSi).
 */
void touchInit(void);

/**
 * @brief Read a touch X/Y position into the provided buffer.
 */
void touchReadXY(touchPosition *touchPos);

/**
 * @brief Returns true if the screen is currently being touched.
 */
bool touchPenDown(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_TOUCH_H__
