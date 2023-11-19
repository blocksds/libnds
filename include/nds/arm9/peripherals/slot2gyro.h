// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_ARM9_PERIPHERALS_SLOT2GYRO_H__
#define LIBNDS_ARM9_PERIPHERALS_SLOT2GYRO_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Perform a gyro sensor scan.
/// TODO: Test on hardware.
///
/// @return The read value (0-4095), or -1 on failure.
int peripheralSlot2GyroScan(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_ARM9_PERIPHERALS_SLOT2GYRO_H__
