// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 zeromus
// Copyright (C) 2011 Dave Murphy (WinterMute)

/// @file nds/arm9/guitarGrip.h
///
/// @brief guitar grip device slot-2 addon support.

#ifndef LIBNDS_NDS_ARM9_GUITARGRIP_H__
#define LIBNDS_NDS_ARM9_GUITARGRIP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <nds/ndstypes.h>

#define GUITARGRIP_GREEN    BIT(6)
#define GUITARGRIP_RED      BIT(5)
#define GUITARGRIP_YELLOW   BIT(4)
#define GUITARGRIP_BLUE     BIT(3)

/// Check for the guitar grip.
///
/// @return Returns true if there is a guitar grip in the slot-2.
bool guitarGripIsInserted(void);

/// Obtain the current guitar grip state.
///
/// Call this function once per main loop to use the guitarGrip functions.
void guitarGripScanKeys(void);

/// Obtains the keys currently held in the guitar grip.
///
/// @return Returns a bitmask of the currently held keys.
u8 guitarGripKeysHeld(void);

/// Obtains the keys that have just been pressed in the guitar grip.
///
/// @return Returns a bitmask of the currently held keys.
u16 guitarGripKeysDown(void);

/// Obtains the keys that have just been released in the guitar grip.
///
/// @return Returns a bitmask of the currently held keys.
u16 guitarGripKeysUp(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_GUITARGRIP_H__
