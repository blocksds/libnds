// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 Tobias Weyand (0xtob)

/// @file nds/arm9/piano.h
///
/// @brief NDS Easy Piano option pack support.

#ifndef LIBNDS_NDS_ARM9_PIANO_H__
#define LIBNDS_NDS_ARM9_PIANO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include <nds/ndstypes.h>

#define PIANO_PAK   (*(vu16 *)0x09FFFFFE)

#define PIANO_C     BIT(0)
#define PIANO_CS    BIT(1)
#define PIANO_D     BIT(2)
#define PIANO_DS    BIT(3)
#define PIANO_E     BIT(4)
#define PIANO_F     BIT(5)
#define PIANO_FS    BIT(6)
#define PIANO_G     BIT(7)
#define PIANO_GS    BIT(8)
#define PIANO_A     BIT(9)
#define PIANO_AS    BIT(10)
#define PIANO_B     BIT(13)
#define PIANO_C2    BIT(14)

/// Check for piano option pack.
///
/// @return true if the cart in the GBA slot is the piano option pack.
bool pianoIsInserted(void);

/// Obtain the current piano state.
///
/// Call this function once per main loop to use the piano functions.
void pianoScanKeys(void);

/// Obtains the current piano keys held state.
///
/// @return Bitmask of keys.
u16 pianoKeysHeld(void);

/// Obtains the current piano keys pressed state.
///
/// @return Bitmask of keys.
u16 pianoKeysDown(void);

/// Obtains the current piano keys released state.
///
/// @return Bitmask of keys.
u16 pianoKeysUp(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_PIANO_H__
