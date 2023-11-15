// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2010 Jason Rogers (dovoto)
// Copyright (C) 2010 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_INPUT_H__
#define LIBNDS_NDS_INPUT_H__

#include <nds/ndstypes.h>

/// @file nds/input.h
///
/// @brief common values for keypad input.
///
/// Common values that can be used on both the ARM9 and ARM7.

/// Enum values for the keypad buttons.
typedef enum KEYPAD_BITS {
    KEY_A      = BIT(0),  ///< Keypad A button.
    KEY_B      = BIT(1),  ///< Keypad B button.
    KEY_SELECT = BIT(2),  ///< Keypad SELECT button.
    KEY_START  = BIT(3),  ///< Keypad START button.
    KEY_RIGHT  = BIT(4),  ///< Keypad RIGHT button.
    KEY_LEFT   = BIT(5),  ///< Keypad LEFT button.
    KEY_UP     = BIT(6),  ///< Keypad UP button.
    KEY_DOWN   = BIT(7),  ///< Keypad DOWN button.
    KEY_R      = BIT(8),  ///< Right shoulder button.
    KEY_L      = BIT(9),  ///< Left shoulder button.
    KEY_X      = BIT(10), ///< Keypad X button.
    KEY_Y      = BIT(11), ///< Keypad Y button.
    KEY_TOUCH  = BIT(12), ///< Touchscreen pendown.
    KEY_LID    = BIT(13), ///< Lid state.
    KEY_DEBUG  = BIT(14), ///< Debug button.
} KEYPAD_BITS;

/// Key input register.
///
/// On the ARM9, the hinge "button", the touch status, and the X and Y buttons
/// cannot be accessed directly.
#define REG_KEYINPUT (*(vuint16 *)0x04000130)

/// Key input control register.
#define REG_KEYCNT (*(vuint16 *)0x04000132)

#define KEYXY_X         BIT(0) ///< ARM7: Keypad X button.
#define KEYXY_Y         BIT(1) ///< ARM7: Keypad Y button.
#define KEYXY_DEBUG     BIT(3) ///< ARM7: Debug button.
#define KEYXY_TOUCH     BIT(6) ///< ARM7: Touchscreen pendown.
#define KEYXY_LID       BIT(7) ///< ARM7: Lid state.

#endif // LIBNDS_NDS_INPUT_H__
