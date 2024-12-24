// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007 Jason Rogers (dovoto)

/// @file nds/arm9/keyboard.h
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

#ifdef __cplusplus
extern "C" {
#endif

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
typedef struct KeyMap
{
    const u16 *mapDataPressed;  ///< The map for keys pressed
    const u16 *mapDataReleased; ///< The map for keys released
    const s16 *keymap; ///< The lookup table for x y grid location to corresponding key
    u8 width;          ///< Width of the keyboard in grid spaces
    u8 height;         ///< Height of the keyboard in grid spaces
} KeyMap;

/// Describes a keyboard.
typedef struct Keyboard
{
    /// Keyboard scroll speed on hide and show in pixels per frame.
    ///
    /// Must be positive. 0 means that the keyboard is shown/hidden right away.
    u8 scrollSpeed;

    u8 grid_width;       ///< Grid width, used to translate x coordinate to keymap
    u8 grid_height;      ///< Grid height, used to translate y coordinate to keymap
    bool shifted;        ///< If shifted, true (e.g. if you want the first char to be uppercase).
    KeyboardState state; ///< The state of the keyboard
    const KeyMap *mappings[4]; ///< Array of 4 keymap pointers, one for every KeyboardState
    //KeyMap *lower;     ///< Keymapping for lower case normal keyboard
    //KeyMap *upper;     ///< Keymapping for shifted upper case normal keyboard
    //KeyMap *numeric;   ///< Keymapping for numeric keypad
    //KeyMap *reduced;   ///< Keymapping for reduced footprint keyboard
    const void *tiles;   ///< Pointer to graphics tiles, cannot exceed 44KB with default base
    u32 tileLen;         ///< Length in bytes of graphics data

    /// Tile offset (in bytes) to load graphics.
    ///
    /// The map must be preadjusted for this offset. TODO: Make this work.
    int tileOffset;

    const void *palette; ///< Pointer to the palette
    u32 paletteLen;      ///< Length in bytes of the palette data

    KeyChangeCallback OnKeyPressed;  ///< Will be called on key press
    KeyChangeCallback OnKeyReleased; ///< Will be called on key release

    bool visible;        ///< If visible, true. Initialized by keyboardInit().
    u8 mapBase;          ///< Map base to be used by the keyboard. Initialized by keyboardInit().
    u8 tileBase;         ///< Tile base to be used by keyboard graphics. Initialized by keyboardInit().
    bool keyboardOnSub;  ///< True if the keyboard is on the sub screen. Initialized by keyboardInit().
    int background;      ///< Background ID used by the keyboard. Initialized by keyboardInit()
    s16 offset_x;        ///< Current X offset of the map. Initialized by keyboardInit()
    s16 offset_y;        ///< Current Y offset of the map. Initialized by keyboardInit()
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

/// Gets the default keyboard.
///
/// @return
///     Returns a read-only pointer to the default keyboard.
WARN_UNUSED_RESULT
const Keyboard *keyboardGetDefault(void);

/// Initializes the keyboard system with the supplied keyboard.
///
/// @note
///     If you pass a custom keyboard struct to this function, make sure that
///     the pointer is never deallocated while the keyboard is in use. That
///     pointer will be used to restore the keyboard to the right state every
///     time it is hidden and shown again on the screen.
///
/// @param keyboard
///     The keyboard struct to initialize (can be NULL).
/// @param layer
///     The background layer to use.
/// @param type
///     The background type to initialize.
/// @param size
///     The background size to initialize.
/// @param mapBase
///     The map base to use for the background.
/// @param tileBase
///     The graphics tile base to use for the background.
/// @param mainDisplay
///     If true the keyboard will render on the main display.
/// @param loadGraphics
///     If true the keyboard graphics will be loaded.
///
/// @return
///     A pointer to the new active keyboard, which you can modify (to modify
///     the key press and key release callbacks, for example).
static inline Keyboard *keyboardInit(const Keyboard *keyboard, int layer, BgType type, BgSize size,
                                     int mapBase, int tileBase, bool mainDisplay, bool loadGraphics)
{
    // Internal function, don't use this directly from outside of libnds
    Keyboard *keyboardInit_call(const Keyboard *keyboard, int layer, BgType type, BgSize size,
                                int mapBase, int tileBase, bool mainDisplay, bool loadGraphics);

    return keyboardInit_call(keyboard == NULL ? keyboardGetDefault() : keyboard, layer, type,
                             size, mapBase, tileBase, mainDisplay, loadGraphics);
}

/// Initializes the default keyboard of libnds.
///
/// Same as calling:
/// ```
/// keyboardInit(NULL, 3, BgType_Text4bpp, BgSize_T_256x512, 20, 0, false, true)
/// ```
///
/// @return
///     A pointer to the new active keyboard, which you can modify (to setup key
///     press and key release callbacks, for example).
Keyboard *keyboardDemoInit(void);

/// De-initializes the keyboard system, if initialized.
///
/// After calling this function you'll need to call keyboardInit() again to use
/// the keyboard, so it is safe to free any struct that you may have allocated
/// (for example, if you're using a custom keyboard).
void keyboardExit(void);

/// Displays the keyboard.
///
/// This will set the state of the keyboard to the original one (the one it had
/// when the keyboard was initialized). If the default state of the keyboard is
/// to show upper-case letters, this will return to that state.
void keyboardShow(void);

/// Hides the keyboard.
///
/// If scrollSpeed has been set to a non-zero value it will scroll it out of the
/// screen. If not, it will hide it right away.
void keyboardHide(void);

/// Returns the ASCII code for the key located at the supplied x and y.
///
/// Will not effect keyboard shift state.
///
/// @param x
///     The pixel x location.
/// @param y
///     The pixel y location.
///
/// @return
///     The key pressed or NOKEY if user pressed outside the keypad.
s16 keyboardGetKey(int x, int y);

/// Reads the input until a the return key is pressed or the maxLen is exceeded.
///
/// @param buffer
///     A buffer to hold the input string
/// @param maxLen
///     The maximum length to read
void keyboardGetString(char *buffer, int maxLen);

/// Waits for user to press a key and returns the key pressed.
///
/// Use keyboardUpdate instead for async operation.
///
/// Remember to call scanKeys() every frame if you use keyboardGetChar().
///
/// @return
///     The key pressed.
s16 keyboardGetChar(void);

/// Processes the keyboard.
///
/// Should be called once per frame when using the keyboard in an async manner.
///
/// Remember to call scanKeys() every frame if you use keyboardUpdate().
///
/// @return
///     The ASCII code of the key pressed or NOKEY if no key was pressed.
s16 keyboardUpdate(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_KEYBOARD_H__
