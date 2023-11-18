// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_ARM9_RUMBLE_H__
#define LIBNDS_NDS_ARM9_RUMBLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <nds/arm9/peripherals/slot2.h>

/// Initialize the rumble device.
///
/// @deprecated Use of peripheralSlot2Init() is recommended instead.
static inline void rumbleInit(void) {
    peripheralSlot2InitDefault();
}

/// Check if a rumble device has been inserted.
///
/// @return True if a rumble device has been inserted, false otherwise.
bool isRumbleInserted(void);

/// Activate the rumble motor.
///
/// @param strength The rumble strength.
void setRumble(uint8_t strength);
#define RUMBLE_STRENGTH_HIGHEST 0xFF

/// Get the maximum rumble strength.
///
/// @return The maximum rumble strength for this device.
uint8_t rumbleGetMaxStrength(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_RUMBLE_H__
