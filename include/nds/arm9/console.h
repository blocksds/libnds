// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/arm9/console.h
///
/// @brief NDS stdout and stderr support.
///
/// Provides stdio integration for printing using <stdio.h> functions.
///
/// ### Regular console
///
/// It is initialized by calling consoleDemoInit() (or consoleInit() to
/// customize the console), and it will simply print messages to the screen of
/// the DS whenever the user sends text to `stdout` (with `printf()`, for
/// example).
///
/// The default console utilizes the sub display, approximatly 4 KiB of VRAM C
/// starting at tile base 3 and 2 KiB of map at map base 22.
///
/// ### Debug console
///
/// The debug console uses `stderr`, and it is initialized by calling
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
/// ### ANSI excape sequences
///
/// The regular console supports ANSI escape sequences. They are special strings
/// that can be sent to printf() and have special effects on the console. For
/// example, `printf("\x1b[2J");` will clear the console.
///
/// Note that in the following strings a `n` means that you can use any positive
/// integer value. It doesn't need to be only one character long. `0` is valid,
/// but `23` is also valid. Also, note that you can use `%d` instead of a
/// hardcoded number. That way you can pass the ANSI escape sequence parameter
/// as an argument to `printf()`:
///
/// ```
/// printf("\x1b[%d;%dm%c", color, intensity, char_to_print);`
/// ```
///
/// The escape sequences supported by libnds are:
///
/// - `[nA`: Move cursor up by `n` chars (stops at the top row).
///
/// - `[nB`: Move cursor down by `n` chars (stops at the bottom row).
///
/// - `[nC`: Move cursor right by `n` chars (stops at the rightmost column).
///
/// - `[nD`: Move cursor left by `n` chars (stops at the leftmost column).
///
/// - `[y;xH` and `[y;xf`: Set cursor to `(x, y)`.
///
///   If the coordinates are too big, the cursor will stop at the rightmost
///   column and the bottom row.
///
/// - `[nJ`: Clear screen.
///
///   If `n` is 0 or it is missing, it will clear from the cursor to the end of
///   the screen. If `n` is 1, it will clear from the beginning of the screen to
///   the cursor. If `n` is 2, it will clear the screen and reset the cursor to
///   (0, 0).
///
/// - `[nK`: Clear line.
///
///   If `n` is 0 or it is missing, it will clear from the cursor to the end of
///   the line. If `n` is 1, it will clear from the beginning of the line to the
///   cursor. If `n` is 2, it will clear the line and reset the cursor to the
///   start of the line.
///
/// - `[s`: Save current cursor position. It will overwrite the previously
///   saved position.
///
/// - `[u`: Restore saved cursor position. It will preserve the saved position
///   in case you want to restore it later.
///
/// - `[c;im`: Change color of the text that will be printed after this.
///
///   Parameter `c` is the color, and parameter `i` is the intensity. Colors go
///   from 30 to 37 (black, red, green, yellow, blue, magenta, cyan, white), and
///   intensity can be 0 or 1 (1 will make the colors brighter).
///
///   Color 39 resets the color back to white: `printf("\x1b[39;0m");`
///
/// A list of all ANSI escape sequences is available here:
/// https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_(Control_Sequence_Introducer)_sequences

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
/// The graphics defined in this struct are loaded by consoleInit() if
/// `loadGraphics` is true, and by consoleSetFont().
///
/// @warning
///     The space character must always be included in the font! It is required
///     by the functions that clear the console, and it is printed when
///     characters that are out of range are sent to the console.
typedef struct ConsoleFont
{
    /// Pointer to the font graphics.
    ///
    /// This pointer can't be NULL.
    const void *gfx;

    /// Pointer to the font palette.
    ///
    /// If this pointer is NULL, the default palettes will be loaded instead.
    /// For 1 BPP and 4BPP fonts, 16 different palettes will be setup to be able
    /// to change the color of the text. For 8 BPP palettes, there is only one
    /// palette, so the only color that is loaded is white.
    const void *pal;

    /// Number of colors in the font palette to be loaded.
    u16 numColors;

    /// Bits per pixel in the font graphics.
    ///
    /// 4 BPP and 8 BPP graphics are loaded as they are provided. 1 BPP fonts
    /// are extended to 4 BPP and then treated like a 4 BPP font.
    u8 bpp;

    /// Offset to the first valid character in the font table.
    ///
    /// This is useful to save space at the beginning of the font, where there
    /// are lots of non-printable ASCII characters.
    u16 asciiOffset;

    /// Number of characters in the font graphics to be loaded.
    u16 numChars;
} ConsoleFont;

/// Console structure used to store the state of a console render context.
///
/// Many of the values in this struct are actually initialized by libnds, and
/// the user should leave them as 0 when consoleInit() is called.
///
/// Default values from consoleGetDefault():
/// ```
/// PrintConsole defaultConsole =
/// {
///     .font =
///     {
///         .gfx = default_fontTiles, // Font tiles
///         .pal = NULL,              // No font palette (use the default palettes)
///         .numColors = 0,
///         .bpp = 1,
///         .asciiOffset = 32,        // First ASCII character in the set
///         .numChars = 96            // Number of characters in the font set
///     },
///
///     .consoleWidth = 32,
///     .consoleHeight = 24,
///     .windowX = 0,
///     .windowY = 0,
///     .windowWidth = 32,
///     .windowHeight = 24,
///     .tabSize = 3,
///     .PrintChar = NULL,
/// };
/// ```
typedef struct PrintConsole
{
    ConsoleFont font; ///< Font of the console.

    /// Pointer to the bg layer map if used. Initialized by consoleInit().
    u16 *fontBgMap;
    /// Pointer to the bg layer graphics if used. Initialized by consoleInit().
    u16 *fontBgGfx;
    /// Palette index where a custom palette is loaded. Initialized by consoleInit().
    u8 fontPalIndex;

    /// Background ID. Initialized by consoleInit().
    int bgId;

    /// Current X location of the cursor. Initialized by consoleInit().
    s16 cursorX;
    /// Current Y location of the cursor. Initialized by consoleInit().
    s16 cursorY;

    /// Internal. Used by "\x1b[s" and "\x1b[u". Initialized by consoleInit().
    s16 prevCursorX;
    /// Internal. Used by "\x1b[s" and "\x1b[u". Initialized by consoleInit().
    s16 prevCursorY;

    u16 consoleWidth;  ///< Width of the console hardware layer in tiles
    u16 consoleHeight; ///< Height of the console hardware layer in tiles

    u16 windowX;      ///< Window X location in tiles
    u16 windowY;      ///< Window Y location in tiles
    u16 windowWidth;  ///< Window width in tiles
    u16 windowHeight; ///< Window height in tiles

    u8 tabSize; ///< Size of a TAB character

    /// Offset to the first graphics tile in background memory (in case your
    /// font is not loaded at a graphics base boundary). Initialized by
    /// consoleInit().
    u16 fontCharOffset;

    /// The current palette used by the engine. Initialized by consoleInit().
    u16 fontCurPal;

    /// Callback for printing a character.
    ///
    /// It should return true if it has handled rendering the graphics. If not,
    /// the print engine will attempt to render via tiles).
    ConsolePrint PrintChar;
} PrintConsole;

/// Console debug devices supported by libnds.
typedef enum
{
    DebugDevice_NULL = 0x0,     ///< Ignores prints to stderr
    DebugDevice_NOCASH = 0x1,   ///< Directs stderr to the no$gba debug window
    DebugDevice_CONSOLE = 0x02  ///< Directs stderr to the DS console
} DebugDevice;

/// Loads the font into the console.
///
/// @param console
///     Pointer to the console to update. If NULL, it will update the current
///     console.
/// @param font
///     The font to load.
void consoleSetFont(PrintConsole *console, ConsoleFont *font);

/// Sets the console cursor position.
///
/// @param console
///     Console to set. If NULL it will set the current console window
/// @param x
///     New X location of the cursor.
/// @param y
///     New Y location of the cursor.
void consoleSetCursor(PrintConsole *console, int x, int y);

/// Moves the console cursor position from its current position.
///
/// @param console
///     Console to set. If NULL it will set the current console window
/// @param deltaX
///     Value to add to the X location of the cursor.
/// @param deltaY
///     Value to add to the Y location of the cursor.
void consoleAddToCursor(PrintConsole *console, int deltaX, int deltaY);

/// Gets the console cursor.
///
/// @param console
///     Console to set. If NULL it will set the current console window
/// @param x
///     Pointer to store the X location of the cursor.
/// @param y
///     Pointer to store the Y location of the cursor.
void consoleGetCursor(PrintConsole *console, int *x, int *y);

/// Colors of the default palettes of libnds.
typedef enum {
    CONSOLE_BLACK           = 0, ///< Black
    CONSOLE_RED             = 1, ///< Red
    CONSOLE_GREEN           = 2, ///< Green
    CONSOLE_YELLOW          = 3, ///< Yellow
    CONSOLE_BLUE            = 4, ///< Blue
    CONSOLE_MAGENTA         = 5, ///< Magenta
    CONSOLE_CYAN            = 6, ///< Cyan
    CONSOLE_LIGHT_GRAY      = 7, ///< Light gray

    CONSOLE_GRAY            = 8,  ///< Gray
    CONSOLE_LIGHT_RED       = 9,  ///< Light red
    CONSOLE_LIGHT_GREEN     = 10, ///< Light green
    CONSOLE_LIGHT_YELLOW    = 11, ///< Light yellow
    CONSOLE_LIGHT_BLUE      = 12, ///< Light blue
    CONSOLE_LIGHT_MAGENTA   = 13, ///< Light magenta
    CONSOLE_LIGHT_CYAN      = 14, ///< Light cyan
    CONSOLE_WHITE           = 15, ///< White

    CONSOLE_DEFAULT         = 16, ///< Default color (white)
} ConsoleColor;

/// Sets the color to use to print new text.
///
/// @note
///     This only works for 4 BPP backgrounds. 8 BPP extended palettes are not
///     supported.
///
/// @param console
///     Console to set. If NULL it will set the current console window
/// @param color
///     Color to be used for new text.
void consoleSetColor(PrintConsole *console, ConsoleColor color);

/// Sets the print window dimensions.
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
///     A read-only pointer to the console with the default values.
const PrintConsole *consoleGetDefault(void);

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
/// @param palIndex
///     For 4 BPP (and 1 BPP) fonts. Palette index to load custom palettes to.
/// @param fontCharOffset
///     How many characters to skip in the tile base slot. This can be used to
///     load multiple fonts to the same slot. One of them can set this to 0, and
///     the other one can set it to 128 so that they don't overlap. Note that
///     tile map slots can't be used by multiple consoles, they all need to be
///     independent.
/// @param mainDisplay
///     If true main engine is used, otherwise false.
/// @param loadGraphics
///     If true the default font graphics will be loaded into the layer.
///
/// @return
///     A pointer to the current console.
PrintConsole *consoleInitEx(PrintConsole *console, int layer, BgType type, BgSize size,
                            int mapBase, int tileBase, int palIndex, int fontCharOffset,
                            bool mainDisplay, bool loadGraphics);

/// Initialise the console.
///
/// @note
///     If you want a more customizable version of this function, check
///     consoleInitEx().
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
