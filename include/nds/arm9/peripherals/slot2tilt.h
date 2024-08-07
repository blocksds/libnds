// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_ARM9_PERIPHERALS_SLOT2TILT_H__
#define LIBNDS_ARM9_PERIPHERALS_SLOT2TILT_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/arm9/peripherals/slot2tilt.h
///
/// @brief Slot-2 tilt sensor.

#include <stdbool.h>
#include <stdint.h>

typedef struct slot2TiltPosition
{
    u16 x;   ///< Raw X value
    u16 y;   ///< Raw Y value
} slot2TiltPosition;

/// Start a new tilt sensor measurement.
/// This is required before a successful read can be performed.
///
/// @return
///     True on success.
bool peripheralSlot2TiltStart(void);

/// Read the tilt sensor values, if the sensor measurement has finished.
///
/// @param data
///     Tilt sensor position data.
///
/// @return
///     True if new values are available.
///
/// @see peripheralSlot2TiltStart
bool peripheralSlot2TiltRead(slot2TiltPosition *data);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_ARM9_PERIPHERALS_SLOT2TILT_H__
