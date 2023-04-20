// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file console.h
///
/// @brief NDS stdio support.
///
/// Provides stdio integration for printing to the DS screen as well as debug
/// print functionality provided by stderr.
///
/// General usage is to initialize the console by calling consoleDemoInit() or
/// to customize the console usage by calling consoleInit()
///
/// The default instance utilizes the sub display, approximatly 4 KiB of VRAM C
/// starting at tile base 3 and 2 KiB of map at map base 22.
///
/// Debug printing is performed by initializing the debug console via
/// consoleDebugInit() as follows:
///
/// <pre>
/// consoleDebugInit(DebugDevice_NOCASH);
/// fprintf(stderr, "debug message in no$gba window %i", stuff);
/// </pre>
///
/// OR
///
/// <pre>
/// consoleDebugInit(DebugDevice_CONSOLE);
/// fprintf(stderr, "debug message on DS console screen");
/// </pre>
///
/// The print console must be initialized to use DB_CONSOLE.

#ifndef LIBNDS_NDS_ARM9_CONSOLE_H__
#define LIBNDS_NDS_ARM9_CONSOLE_H__

#include <nds/arm9/background.h>
#include <nds/ndstypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (* ConsolePrint)(void* con, char c);

/// A font struct for the console.
///
/// If convertSingleColor is true, the font is treated as a single color font
/// where all non zero pixels are set to a value of 15 or 255 (4bpp / 8bpp
/// respectivly). This ensures only one palette entry is utilized for font
/// rendering.
typedef struct ConsoleFont
{
    u16 *gfx; ///< A pointer to the font graphics (will be loaded by consoleInit() if loadGraphics is true)
    u16 *pal; ///< A pointer to the font palette (will be loaded by consoleInit() if loadGraphics is true)
    u16 numColors;   ///< Number of colors in the font palette
    u8 bpp;          ///< Bits per pixel in the font graphics
    u16 asciiOffset; ///< Offset to the first valid character in the font table
    u16 numChars;    ///< Number of characters in the font graphics
    bool convertSingleColor; ///< Convert from 1bpp font
} ConsoleFont;

/// Console structure used to store the state of a console render context.
///
/// Default values from consoleGetDefault();
/// <pre>
/// PrintConsole defaultConsole =
/// {
///     // Font:
///     {
///         (u16 *)default_font_bin, // Font gfx
///         0,      // Font palette
///         0,      // Font color count
///         4,      // bpp
///         0,      // First ascii character in the set
///         128,    // Number of characters in the font set
///         true,   // Convert to single color
///     },
///     0,      // Font background map
///     0,      // Font background gfx
///     31,     // Map base
///     0,      // Char base
///     0,      // BG layer in use
///     -1,     // BG id
///     0, 0,   // CursorX cursorY
///     0, 0,   // PrevcursorX prevcursorY
///     32,     // Console width
///     24,     // Console height
///     0,      // Window x
///     0,      // Window y
///     32,     // Window width
///     24,     // Window height
///     3,      // Tab size
///     0,      // Font character offset
///     0,      // Selected palette
///     0,      // Print callback
///     false,  // Console initialized
///     true,   // Load graphics
/// };
/// </pre>
typedef struct PrintConsole
{
    ConsoleFont font;   ///< Font of the console.

    u16 *fontBgMap;     ///< Pointer to the bg layer map if used. Is set by
                        /// bgInit if bgId is valid

    u16 *fontBgGfx;     ///< Pointer to the bg layer graphics if used. Is set by
                        /// bgInit if bgId is valid

    u8 mapBase;         ///< Map base set by console init based on background setup
    u8 gfxBase;         ///< Tile graphics base set by console init based on
                        /// background setup

    u8 bgLayer;         ///< Bg layer used by the background
    int bgId;           ///< bgId, should be set with a call to bgInit() or bgInitSub()

    int cursorX;        ///< Current X location of the cursor (as a tile offset by default)
    int cursorY;        ///< Current Y location of the cursor (as a tile offset by default)

    int prevCursorX;    ///< Internal state
    int prevCursorY;    ///< Internal state

    int consoleWidth;   ///< Width of the console hardware layer in tiles
    int consoleHeight;  ///< Height of the console hardware layer in tiles

    int windowX;        ///< Window X location in tiles (not implemented)
    int windowY;        ///< Window Y location in tiles (not implemented)
    int windowWidth;    ///< Window width in tiles (not implemented)
    int windowHeight;   ///< Window height in tiles (not implemented)

    int tabSize;        ///< Size of a tab

    u16 fontCharOffset; ///< Offset to the first graphics tile in background
                        /// memory (in case your font is not loaded at a
                        /// graphics base boundary)

    u16 fontCurPal;     ///< The current palette used by the engine (only
                        /// applies to 4bpp text backgrounds)

    ConsolePrint PrintChar;  ///< Callback for printing a character. It should
                             /// return true if it has handled rendering the
                             /// graphics (else the print engine will attempt to
                             /// render via tiles)

    bool consoleInitialised; ///< True if the console is initialized
    bool loadGraphics;       ///< True if consoleInit should attempt to load
                             /// font graphics into background memory
} PrintConsole;

/// Console debug devices supported by libnds.
typedef enum
{
    DebugDevice_NULL = 0x0,     ///< Ignores prints to stderr
    DebugDevice_NOCASH = 0x1,   ///< Directs stderr to the no$gba debug window
    DebugDevice_CONSOLE = 0x02  ///< Directs stderr to the DS console window
} DebugDevice;

/// Loads the font into the console.
///
/// @param console Pointer to the console to update. If NULL, it will update the
///                current console.
/// @param font The font to load.
void consoleSetFont(PrintConsole *console, ConsoleFont *font);

/// Sets the print window
///
/// @param console Console to set. If NULL it will set the current console
///                window
/// @param x X location of the window.
/// @param y Y location of the window.
/// @param width Width of the window.
/// @param height Height of the window.
void consoleSetWindow(PrintConsole *console, int x, int y, int width, int height);

/// Gets a pointer to the console with the default values.
///
/// This should only be used when using a single console or without changing the
/// console that is returned, otherwise use consoleInit().
///
/// @return A pointer to the console with the default values.
PrintConsole *consoleGetDefault(void);

/// Make the specified console the render target.
///
/// @param console A pointer to the console struct (must have been initialized
///                with consoleInit(PrintConsole* console)
/// @return A pointer to the previous console.
PrintConsole *consoleSelect(PrintConsole* console);

/// Initialise the console.
///
/// @param console A pointer to the console data to initialze (if it's NULL, the
///                default console will be used).
/// @param layer Background layer to use.
/// @param type Type of the background.
/// @param size Size of the background.
/// @param mapBase Map base.
/// @param tileBase Tile graphics base.
/// @param mainDisplay If true main engine is used, otherwise false.
/// @param loadGraphics If true the default font graphics will be loaded into
///                     the layer.
/// @return A pointer to the current console.
PrintConsole *consoleInit(PrintConsole *console, int layer, BgType type, BgSize size,
                          int mapBase, int tileBase, bool mainDisplay, bool loadGraphics);

/// Initialize the console to a default state for prototyping.
///
/// This function sets the console to use sub display, VRAM_C, and BG0 and
/// enables MODE_0_2D on the sub display. It is intended for use in prototyping
/// applications which need print ability and not actual game use. Print
/// functionality can be utilized with just this call.
///
/// @return A pointer to the current PrintConsole.
PrintConsole *consoleDemoInit(void);

/// Clears the screan by using printf("\x1b[2J");
void consoleClear(void);

/// Initializes the debug console output on stderr to the specified device.
///
/// @param device The debug device (or devices) to output debug print to.
void consoleDebugInit(DebugDevice device);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_CONSOLE_H__
