// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_ARM9_PERIPHERALS_SLOT2TILT_H__
#define LIBNDS_ARM9_PERIPHERALS_SLOT2TILT_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Read the X axis value of the tilt sensor.
uint16_t peripheralSlot2TiltGetX(void);

/// Read the Y axis value of the tilt sensor.
uint16_t peripheralSlot2TiltGetY(void);

/// Update the tilt sensor values.
/// It is recommended to run this function once per VBlank.
///
/// @return True if new values are available.
bool peripheralSlot2TiltUpdate(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_ARM9_PERIPHERALS_SLOT2TILT_H__
