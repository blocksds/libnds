// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Christian Auby (DesktopMan)

#ifndef LIBNDS_NDS_ARM9_INPUT_H__
#define LIBNDS_NDS_ARM9_INPUT_H__

#ifdef __cplusplus
extern "C" {
#endif

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

/// Obtains the current keypad state.
///
/// Call this function once per main loop in order to use the keypad functions.
void scanKeys(void);

/// Obtains the current keypad state.
///
/// Call this function to get keypad state without affecting state of other key
/// functions (keysUp keysHeld etc...).
///
/// @deprecated
///     This function isn't safe. Normally scanKeys() reads the current state of
///     the keys (from the ARM7 and ARM9) and saves the current state in an
///     atomic way to prevent race conditions. keysCurrent() doesn't work in an
///     atomic way, so it's likely to cause bugs. Use scanKeys() and keysHeld()
///     instead.
///
/// @return
///     Bitmask of keys that are pressed.
LIBNDS_DEPRECATED uint32_t keysCurrent(void);

/// Obtains the current keypad held state.
///
/// @return
///     Bitmask of keys that are pressed.
uint32_t keysHeld(void);

/// Obtains the keys that have been pressed right now.
///
/// @return
///     Bitmask of keys that have just been pressed.
uint32_t keysDown(void);

/// Obtains the keys that have been held for long enough to repeat the press.
///
/// keysDownRepeat() doesn't keep track of how long ago each individual key was
/// pressed or released. It keeps track of the last time any key changed, and it
/// returns that mask whenever the delay counter reaches the end.
///
/// Whenever a key is pressed or released the new mask is saved. While the
/// pressed buttons match the saved mask, a counter ticks. This counter goes
/// back to the beginning whenever the mask changes due to a key being pressed
/// or released.
///
/// The first repetition comes after a starting delay set by keysSetRepeat().
/// After the first delay there is a different repeated delay (usually shorter)
/// that will retrigger the repeated presses.
///
/// @warning
///     This function clears the state of repeated key presses. Call this
///     function only once after each call to scanKeys().
/// @return
///     Bitmask of keys that have been held for long enough to repeat the press.
uint32_t keysDownRepeat(void);

/// Sets the key repeat parameters.
///
/// @param setDelay
///     Number of scanKeys() calls before keys start to repeat.
/// @param setRepeat
///     Number of scanKeys() calls before keys repeat.
void keysSetRepeat(u8 setDelay, u8 setRepeat);

/// Obtains the keys that have just been released.
///
/// @return
///     Bitmask of keys that have just been released.
uint32_t keysUp(void);

/// Obtains the current touchpad state.
///
/// @param data
///     A touchPosition pointer which will be filled by the function.
void touchRead(touchPosition *data);

// Old way of reading the touchpad state.
static inline LIBNDS_DEPRECATED touchPosition touchReadXY(void)
{
    touchPosition touchPos;
    touchRead(&touchPos);
    return touchPos;
}

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_INPUT_H__
