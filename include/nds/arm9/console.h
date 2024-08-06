// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/arm9/console.h
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
/// ```
/// consoleDebugInit(DebugDevice_NOCASH);
/// fprintf(stderr, "debug message in no$gba window %i", stuff);
/// ```
///
/// OR
///
/// ```
/// consoleDebugInit(DebugDevice_CONSOLE);
/// fprintf(stderr, "debug message on DS console screen");
/// ```
///
/// The print console must be initialized to use DB_CONSOLE.

#ifndef LIBNDS_NDS_ARM9_CONSOLE_H__
#define LIBNDS_NDS_ARM9_CONSOLE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include <nds/arm9/background.h>
#include <nds/ndstypes.h>

/// Function type used by the PrintConsole struct to send characters to the
/// console.
typedef bool (* ConsolePrint)(void *con, char c);

/// Function type used by libnds to redirect characters sent to stdout and
/// stderr (skipping the call to the ConsolePrint handler).
typedef ssize_t (* ConsoleOutFn)(const char *ptr, size_t len);

/// A font struct for the console.
///
/// If convertSingleColor is true, the font is treated as a single color font
/// where all non zero pixels are set to a value of 15 or 255 (4bpp / 8bpp
/// respectivly). This ensures only one palette entry is utilized for font
/// rendering.
typedef struct ConsoleFont
{
    /// Pointer to the font graphics.
    ///
    /// They will be loaded by consoleInit() if loadGraphics is true.
    const void *gfx;

    /// Pointer to the font palette.
    ///
    /// They will be loaded by consoleInit() if loadGraphics is true.
    const void *pal;

    u16 numColors;   ///< Number of colors in the font palette
    u8 bpp;          ///< Bits per pixel in the font graphics
    u16 asciiOffset; ///< Offset to the first valid character in the font table
    u16 numChars;    ///< Number of characters in the font graphics
} ConsoleFont;

/// Console structure used to store the state of a console render context.
///
/// Default values from consoleGetDefault();
/// ```
/// PrintConsole defaultConsole =
/// {
///     .font =
///     {
///         .gfx = default_fontTiles, // font gfx
///         .pal = NULL,              // font palette
///         .numColors = 0,           // font color count
///         .bpp = 1,
///         .asciiOffset = 32,        // first ascii character in the set
///         .numChars = 96            // number of characters in the font set
///     },
///
///     .consoleWidth = 32,
///     .consoleHeight = 24,
///     .windowX = 0,
///     .windowY = 0,
///     .windowWidth = 32,
///     .windowHeight = 24,
///     .tabSize = 3,
///     .fontCharOffset = 0,
///     .fontCurPal = 0,
///     .PrintChar = NULL,
///     .consoleInitialised = false,
///     .loadGraphics = true,
/// };
/// ```
typedef struct PrintConsole
{
    ConsoleFont font; ///< Font of the console.

    /// Pointer to the bg layer map if used. Initialized by consoleInit().
    u16 *fontBgMap;
    /// Pointer to the bg layer graphics if used. Initialized by consoleInit().
    u16 *fontBgGfx;

    int bgId;   ///< Background ID. Initialized by consoleInit().

    /// Current X location of the cursor. Initialized by consoleInit().
    s16 cursorX;
    /// Current Y location of the cursor. Initialized by consoleInit().
    s16 cursorY;

    s16 prevCursorX; ///< Internal state. Initialized by consoleInit().
    s16 prevCursorY; ///< Internal state. Initialized by consoleInit().

    u16 consoleWidth;  ///< Width of the console hardware layer in tiles
    u16 consoleHeight; ///< Height of the console hardware layer in tiles

    u16 windowX;      ///< Window X location in tiles
    u16 windowY;      ///< Window Y location in tiles
    u16 windowWidth;  ///< Window width in tiles
    u16 windowHeight; ///< Window height in tiles

    u8 tabSize; ///< Size of a tab

    /// Offset to the first graphics tile in background memory (in case your
    /// font is not loaded at a graphics base boundary)
    u16 fontCharOffset;

    /// The current palette used by the engine (only applies to 4bpp text
    /// backgrounds)
    u16 fontCurPal;

    /// Callback for printing a character.
    ///
    /// It should return true if it has handled rendering the graphics. If not,
    /// the print engine will attempt to render via tiles)
    ConsolePrint PrintChar;

    /// This is set to true by consoleInit()
    bool consoleInitialised;

    /// Set this to true if consoleInit() should attempt to load font graphics
    /// into background VRAM.
    bool loadGraphics;
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
/// @param console
///     Pointer to the console to update. If NULL, it will update the current
///     console.
/// @param font
///     The font to load.
void consoleSetFont(PrintConsole *console, ConsoleFont *font);

/// Sets the print window
///
/// @param console
///     Console to set. If NULL it will set the current console window
/// @param x
///     X location of the window.
/// @param y
///     Y location of the window.
/// @param width
///     Width of the window.
/// @param height
///     Height of the window.
void consoleSetWindow(PrintConsole *console, int x, int y, int width, int height);

/// Gets a pointer to the console with the default values.
///
/// This should only be used when using a single console or without changing the
/// console that is returned, otherwise use consoleInit().
///
/// @return
///     A pointer to the console with the default values.
PrintConsole *consoleGetDefault(void);

/// Make the specified console the render target.
///
/// @param console
///     A pointer to the console struct (must have been initialized with
///     consoleInit(PrintConsole* console)
///
/// @return
///     A pointer to the previous console.
PrintConsole *consoleSelect(PrintConsole *console);

/// Initialise the console.
///
/// @param console
///     A pointer to the console data to initialze (if it's NULL, the default
///     console will be used).
/// @param layer
///     Background layer to use.
/// @param type
///     Type of the background.
/// @param size
///     Size of the background.
/// @param mapBase
///     Map base.
/// @param tileBase
///     Tile graphics base.
/// @param mainDisplay
///     If true main engine is used, otherwise false.
/// @param loadGraphics
///     If true the default font graphics will be loaded into the layer.
///
/// @return
///     A pointer to the current console.
PrintConsole *consoleInit(PrintConsole *console, int layer, BgType type, BgSize size,
                          int mapBase, int tileBase, bool mainDisplay, bool loadGraphics);

/// Initialize the console to a default state for prototyping.
///
/// This function sets the console to use sub display, VRAM_C, and BG0 and
/// enables MODE_0_2D on the sub display. It is intended for use in prototyping
/// applications which need print ability and not actual game use. Print
/// functionality can be utilized with just this call.
///
/// The console initialization is equivalent to:
/// ```
/// consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 22, 3, false, true);
/// ```
///
/// @return
///     A pointer to the current PrintConsole.
PrintConsole *consoleDemoInit(void);

/// Clears the console and returns the cursor to the top left corner.
void consoleClear(void);

/// Prints a character to the default console.
///
/// @param c
///     The character to print.
void consolePrintChar(char c);

/// Initializes the debug console output on stderr to the specified device.
///
/// @param device
///     The debug device (or devices) to output debug print to.
void consoleDebugInit(DebugDevice device);

/// Sets the function where stdout is sent, bypassing the PrintConsole handler.
///
/// To reset it to the libnds console handler, call this function with NULL as
/// an argument.
///
/// @param fn
///     Callback where stdout is sent.
void consoleSetCustomStdout(ConsoleOutFn fn);

/// Sets the function where stderr is sent, bypassing the PrintConsole handler.
///
/// To reset it to the libnds console handler, call this function with NULL as
/// an argument, or call consoleDebugInit().
///
/// @param fn
///     Callback where stderr is sent.
void consoleSetCustomStderr(ConsoleOutFn fn);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_CONSOLE_H__
