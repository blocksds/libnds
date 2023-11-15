// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Mike Parks (BigRedPimp)

/// @file nds/arm9/rumble.h
///
/// @brief NDS rumble option pak support.

#ifndef LIBNDS_NDS_ARM9_RUMBLE_H__
#define LIBNDS_NDS_ARM9_RUMBLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <nds/ndstypes.h>

#define RUMBLE_PAK          (*(vuint16 *)0x08000000)
#define WARIOWARE_PAK       (*(vuint16 *)0x080000C4)
#define WARIOWARE_ENABLE    (*(vuint16 *)0x080000C6)

/// Supported rumble cartridges.
typedef enum {
    RUMBLE_TYPE_UNKNOWN,    ///< Rumble detection hasn't run
    RUMBLE_TYPE_NONE,       ///< No rumble detected
    RUMBLE_TYPE_PAK,        ///< DS Rumble Pak
    RUMBLE_TYPE_GBA,        ///< Rumble included as part of GBA game cartridges
    RUMBLE_TYPE_MAGUKIDDO,  ///< Rumble/sensor cartridge bundled with Magukiddo
    RUMBLE_TYPE_SC_RUMBLE   ///< Rumble included with some SuperCard models
} RUMBLE_TYPE;

/// Initializes any detected supported rumble cart.
void rumbleInit(void);

/// Returns the type of the detected rumble cart.
///
/// @return The type of the rumble cart.
RUMBLE_TYPE rumbleGetType(void);

/// Forces a specific rumble cart type.
///
/// If the cartridge isn't actually present, this won't work.
///
/// @param type Rumble type to force.
void rumbleSetType(RUMBLE_TYPE type);

/// Check for rumble option pak.
///
/// @return Returns true if the cart in the GBA slot is a Rumble option pak.
bool isRumbleInserted(void);

/// Fires the rumble actuator.
///
/// @param position Alternates position of the actuator in the pak (ON/OFF).
void setRumble(bool position);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_RUMBLE_H__
