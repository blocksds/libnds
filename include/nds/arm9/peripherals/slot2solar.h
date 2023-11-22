// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_ARM9_PERIPHERALS_SLOT2SOLAR_H__
#define LIBNDS_ARM9_PERIPHERALS_SLOT2SOLAR_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/arm9/peripherals/slot2solar.h
///
/// @brief Slot-2 solar sensor.

#include <stdbool.h>
#include <stdint.h>

/// Perform a "fast" solar sensor scan.
/// Note that this function stalls IRQs.
/// TODO: Test on hardware.
///
/// @return The detected brightness (0 ~ 255, higher is darker),
/// or -1 on failure/timeout.
int peripheralSlot2SolarScanFast(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_ARM9_PERIPHERALS_SLOT2SOLAR_H__
