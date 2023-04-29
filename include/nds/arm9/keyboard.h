// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2007 Jason Rogers (dovoto)

/// @file keyboard.h
///
/// @brief Integration of stdin with a simple keyboard.
///
/// The keyboard component allows the use of a default keyboard via stdin as
/// well as direct via the functions exposed below. The default behavior is a
/// hidden keyboard that shows on a call to scanf(stdin, ...).
///
/// By default the keyboard uses background 3 of the sub display, consumes
/// approximatly 40 KiB of background VRAM begining at tile base 0 and 4 KiB of
/// map stored at map base 20. The default is designed to work alongside an
/// instance of the demo console.
///
/// To customize keyboard behavior and resource usage modify the keyboard
/// structure returned by keyboardGetDefault() or create your own keyboard.

#ifndef LIBNDS_NDS_ARM9_KEYBOARD_H__
#define LIBNDS_NDS_ARM9_KEYBOARD_H__

#include <nds/arm9/background.h>
#include <nds/ndstypes.h>

/// Callback function pointer when a key changes.
typedef void (*KeyChangeCallback)(int key);

/// States the keyboard can be in, currently only Lower and Upper supported.
typedef enum
{
    Lower = 0,   ///< Normal keyboard display (lowercase letters)
    Upper = 1,   ///< Caps lock held
    Numeric = 2, ///< Numeric only keypad (not provided by the default keyboard)
    Reduced = 3  ///< Reduced footprint keyboard (not provided by the default keyboard)
} KeyboardState;

/// Defines a key mapping.
typedef struct KeyMap {
    const u16 *mapDataPressed;  ///< The map for keys pressed
    const u16 *mapDataReleased; ///< The map for keys released
    const s16 *keymap; ///< The lookup table for x y grid location to corresponding key
    int width;         ///< Width of the keyboard in grid spaces
    int height;        ///< Height of the keyboard in grid spaces
} KeyMap;

/// Describes a keyboard.
typedef struct Keyboard {
    int background;      ///< Background use, after init() this contains the background ID
    int keyboardOnSub;   ///< boolean to determine if keyboard is on sub screen or main
    int offset_x;        ///< X offset of the map, can be used to center a custom keyboard
    int offset_y;        ///< Y offset of the map, can be used to center a custom keyboard
    int grid_width;      ///< Grid width, used to translate x coordinate to keymap
    int grid_height;     ///< Grid height, used to translate y coordinate to keymap
    KeyboardState state; ///< The state of the keyboard
    int shifted;         ///< If shifted, true
    int visible;         ///< If visible, true
    KeyMap *mappings[4]; ///< Array of 4 keymap pointers, one for every keyboard state
    //KeyMap *lower;     ///< Keymapping for lower case normal keyboard
    //KeyMap *upper;     ///< Keymapping for shifted upper case normal keyboard
    //KeyMap *numeric;   ///< Keymapping for numeric keypad
    //KeyMap *reduced;   ///< Keymapping for reduced footprint keyboard
    const u16 *tiles;    ///< Pointer to graphics tiles, cannot exceed 44KB with default base
    u32 tileLen;         ///< Length in bytes of graphics data
    const u16 *palette;  ///< Pointer to the palette
    u32 paletteLen;      ///< Length in bytes of the palette data
    int mapBase;         ///< Map base to be used by the keyboard
    int tileBase;        ///< Tile base to be used by keyboard graphics
    int tileOffset;      ///< Tile offset (in bytes) to load graphics (the map
                         /// must be preadjusted for this offset)

    u32 scrollSpeed;     ///< Keyboard scroll speed on hide and show in pixels
                         /// per frame (must be positive; 0 == instant on).

    KeyChangeCallback OnKeyPressed;  ///< Will be called on key press
    KeyChangeCallback OnKeyReleased; ///< Will be called on key release
} Keyboard;


/// Enum values for the keyboard control keys.
///
/// Negative values are keys with no sensible ASCII representation. Numbers are
/// chosen to mimic ASCII control sequences.
typedef enum
{
    NOKEY         = -1,  ///< No key was pressed
    DVK_FOLD      = -23, ///< Fold key (top left on the default keyboard)
    DVK_TAB       =  9,  ///< Tab key
    DVK_BACKSPACE =  8,  ///< Backspace
    DVK_CAPS      = -15, ///< Caps key
    DVK_SHIFT     = -14, ///< Shift key
    DVK_SPACE     =  32, ///< Space key
    DVK_MENU      = -5,  ///< Menu key
    DVK_ENTER     =  10, ///< Enter key
    DVK_CTRL      = -16, ///< Ctrl key
    DVK_UP        = -17, ///< Up key
    DVK_RIGHT     = -18, ///< Right key
    DVK_DOWN      = -19, ///< Down key
    DVK_LEFT      = -20, ///< Left key
    DVK_ALT       = -26  ///< Alt key
} Keys;

#ifdef __cplusplus
extern "C" {
#endif

/// Gets the default keyboard.
///
/// @return Returns the default keyboard.
Keyboard *keyboardGetDefault(void);

/// Initializes the keyboard system with the supplied keyboard.
///
/// @param keyboard The keyboard struct to initialize (can be NULL).
/// @param layer The background layer to use.
/// @param type The background type to initialize.
/// @param size The background size to initialize.
/// @param mapBase The map base to use for the background.
/// @param tileBase The graphics tile base to use for the background.
/// @param mainDisplay If true the keyboard will render on the main display.
/// @param loadGraphics If true the keyboard graphics will be loaded.
/// @return Returns the initialized keyboard struct.
Keyboard *keyboardInit(Keyboard *keyboard, int layer, BgType type, BgSize size,
                       int mapBase, int tileBase, bool mainDisplay, bool loadGraphics);

/// Initializes the keyboard with default options.
///
/// Same as calling:
/// ```
/// keyboardInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x512, 20, 0, false, true)
/// ```
///
/// @return A pointer to the current keyboard.
Keyboard* keyboardDemoInit(void);

/// Displays the keyboard.
void keyboardShow(void);

/// Hides the keyboard.
void keyboardHide(void);

/// Returns the ASCII code for the key located at the supplied x and y.
///
/// Will not effect keyboard shift state.
///
/// @param x The pixel x location.
/// @param y The pixel y location.
/// @return The key pressed or NOKEY if user pressed outside the keypad.
s16 keyboardGetKey(int x, int y);

/// Reads the input until a the return key is pressed or the maxLen is exceeded.
///
/// @param buffer a buffer to hold the input string
/// @param maxLen the maximum length to read
void keyboardGetString(char *buffer, int maxLen);

/// Waits for user to press a key and returns the key pressed.
///
/// Use keyboardUpdate instead for async operation.
///
/// @return The key pressed.
s16 keyboardGetChar(void);

/// Processes the keyboard.
///
/// Should be called once per frame when using the keyboard in an async manner.
///
/// @return the ASCII code of the key pressed or NOKEY if no key was pressed.
s16 keyboardUpdate(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_KEYBOARD_H__
