// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Christian Auby (DesktopMan)

#ifndef LIBNDS_NDS_ARM9_INPUT_H__
#define LIBNDS_NDS_ARM9_INPUT_H__

/// @file nds/arm9/input.h
///
/// @brief NDS button and touchscreen input support.
///
/// The state of the keypad must be read from hardware into memory using
/// scanKeys() whenever you want an updated input state. After reading, call one
/// of the associated "keys" functions to see what event was triggered. These
/// events are computed as the difference between the current and previous key
/// state you read. It's generally a good idea to scan keys frequently to ensure
/// your application's input system is responsive.
///
/// After reading the key state, you will be given an integer representing which
/// keys are in the requested state. To mask of specific buttons, use the key
/// masks described in nds/input.h .
///
/// @see nds/input.h available key masks on the Nintendo DS

#include <nds/input.h>
#include <nds/ndstypes.h>
#include <nds/touch.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Obtains the current keypad state.
///
/// Call this function once per main loop in order to use the keypad functions.
void scanKeys(void);

/// Obtains the current keypad state.
///
/// Call this function to get keypad state without affecting state of other key
/// functions (keysUp keysHeld etc...).
///
/// @return Bitmask of keys that are pressed.
uint32_t keysCurrent(void);

/// Obtains the current keypad held state.
///
/// @return Bitmask of keys that are pressed.
uint32_t keysHeld(void);

/// Obtains the current keypad pressed state.
///
/// @return Bitmask of keys that have just been released.
uint32_t keysDown(void);

/// Obtains the current keypad pressed or repeating state.
///
/// @return Bitmask of keys that have just been pressed or have been held for
/// long enough to repeat the press.
uint32_t keysDownRepeat(void);

/// Sets the key repeat parameters.
///
/// @param setDelay Number of %scanKeys calls before keys start to repeat.
/// @param setRepeat Number of %scanKeys calls before keys repeat.
void keysSetRepeat(u8 setDelay, u8 setRepeat);

/// Obtains the current keypad released state.
///
/// @return Bitmask of keys that have just been released.
uint32_t keysUp(void);

// Old way of reading the touchpad state.
__attribute__((deprecated)) touchPosition touchReadXY(void);

/// Obtains the current touchpad state.
///
/// @param data A touchPosition pointer which will be filled by the function.
void touchRead(touchPosition *data);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_INPUT_H__
