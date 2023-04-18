// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Mike Parks (BigRedPimp)

/// @file rumble.h
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

typedef enum {
   RUMBLE_TYPE_UNKNOWN,
   RUMBLE_TYPE_NONE,
   RUMBLE_TYPE_PAK,         // DS Rumble Pak
   RUMBLE_TYPE_GBA,         // Rumble included as part of GBA game cartridges
   RUMBLE_TYPE_MAGUKIDDO    // Rumble/sensor cartridge bundled with Magukiddo
} RUMBLE_TYPE;

void rumbleInit(void);
RUMBLE_TYPE rumbleGetType(void);
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
