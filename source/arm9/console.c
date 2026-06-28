// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Important note: We can't use any BIOS functions in this file because the
// console is used by the default exception handler. The handler doesn't run in
// CPU user mode, so it doesn't work correctly if it uses BIOS functions.
//
// We shouldn't use any function from <stdio.h> either. This will allow
// developers to use the console with consolePrintChar() without ever requiring
// printf(), or sscanf(), or anything like that.

#include <stdarg.h>
#include <stdlib.h>

#include <nds/arm9/background.h>
#include <nds/arm9/cache.h>
#include <nds/arm9/console.h>
#include <nds/arm9/video.h>
#include <nds/debug.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>

#include "common/libnds_internal.h"
#include "default_font.h"

static const PrintConsole defaultConsole =
{
    .font =
    {
        .gfx = default_fontTiles, // Font tiles
        .pal = NULL,              // No font palette (use the default palettes)
        .numColors = 0,
        .bpp = 1,
        .asciiOffset = 32,        // First ASCII character in the set
        .numChars = 96            // Number of characters in the font set
    },

    // Initialized by consoleInit():
    //
    //.fontBgMap
    //.fontBgGfx
    //.fontPalIndex
    //.bgId
    //.cursorX
    //.cursorY
    //.prevCursorX
    //.prevCursorY
    //.fontCharOffset

    //.fontCurPal
    //.fontCurRgb
    //.backgroundCurPal;
    //.backgroundCurRgb;
    //.sgrIntensity
    //.sgrForegroundIsRgb
    //.sgrBackgroundIsRgb

    .consoleWidth = 32,
    .consoleHeight = 24,
    .windowX = 0,
    .windowY = 0,
    .windowWidth = 32,
    .windowHeight = 24,
    .tabSize = 3,
    .PrintChar = NULL, // Print callback
    .HandleSgrCodes = NULL, // SGR attributes callback
};

#define DEFAULT_CONSOLE_MAP_BASE 22
#define DEFAULT_CONSOLE_GFX_BASE 3
#define DEFAULT_CONSOLE_BG_LAYER 0

static PrintConsole currentCopy;

static PrintConsole *currentConsole = &currentCopy;

const PrintConsole *consoleGetDefault(void)
{
    return &defaultConsole;
}

static void consoleCls(char mode)
{
    int i = 0;
    int colTemp, rowTemp;

    switch (mode)
    {
        case '[':
        case '0':
            colTemp = currentConsole->cursorX;
            rowTemp = currentConsole->cursorY;

            while (i++ < ((currentConsole->windowHeight * currentConsole->windowWidth)
                          - (rowTemp * currentConsole->consoleWidth + colTemp)))
            {
                consolePrintChar(' ');
            }

            currentConsole->cursorX = colTemp;
            currentConsole->cursorY = rowTemp;
            break;

        case '1':
            colTemp = currentConsole->cursorX;
            rowTemp = currentConsole->cursorY;

            currentConsole->cursorY = 0;
            currentConsole->cursorX = 0;

            while (i++ < (rowTemp * currentConsole->windowWidth + colTemp))
                consolePrintChar(' ');

            currentConsole->cursorX = colTemp;
            currentConsole->cursorY = rowTemp;
            break;

        case '2':
            currentConsole->cursorY = 0;
            currentConsole->cursorX = 0;

            while (i++ < currentConsole->windowHeight * currentConsole->windowWidth)
                consolePrintChar(' ');

            currentConsole->cursorY = 0;
            currentConsole->cursorX = 0;
            break;
    }
}

static void consoleClearLine(char mode)
{
    int i = 0;
    int colTemp;

    switch (mode)
    {
        case '[':
        case '0':
            colTemp = currentConsole->cursorX;

            while (i++ < (currentConsole->windowWidth - colTemp))
                consolePrintChar(' ');

            currentConsole->cursorX = colTemp;
            break;

        case '1':
            colTemp = currentConsole->cursorX;

            currentConsole->cursorX = 0;

            while (i++ < (currentConsole->windowWidth - colTemp))
                consolePrintChar(' ');

            currentConsole->cursorX = colTemp;
            break;

        case '2':
            colTemp = currentConsole->cursorX;

            currentConsole->cursorX = 0;

            while (i++ < currentConsole->windowWidth)
                consolePrintChar(' ');

            currentConsole->cursorX = colTemp;
            break;

        default:
            colTemp = currentConsole->cursorX;

            while (i++ < (currentConsole->windowWidth - colTemp))
                consolePrintChar(' ');

            currentConsole->cursorX = colTemp;
            break;
    }
}

ssize_t nocash_write(const char *ptr, size_t len)
{
    for (size_t i = 0; i < len; i++)
        nocash_putc_buffered(ptr[i], NULL);

    return len;
}

static void console_handle_sgr_legacy(void *console,
                                      size_t param_num, unsigned int *params)
{
    // This function is buggy on purpose. It expects commands 30-37 in the first
    // parameter, and commands 0 or 1 in the second one. However, in reality,
    // commands can come in any order. Also, command 0 resets all attributes,
    // including color, but here it's treated as "low brightness". The bugs have
    // been present in libnds for 18 years so it's better to preserve them to
    // not break code that depends on it.

    PrintConsole *con = console;

    if (param_num < 2)
        return;

    int parameter = params[0];
    int intensity = params[1];

    // Only handle 30-37,39 and intensity for the color changes
    parameter -= 30;

    // 39 is the reset code
    if (parameter == 9)
        parameter = 15;
    else if (parameter > 8)
        parameter -= 2;
    else if (intensity)
        parameter += 8;

    if (parameter < 16 && parameter >= 0)
        con->fontCurPal = parameter;
}

static void console_handle_sgr(void *console, size_t param_num, unsigned int *params)
{
    PrintConsole *con = console;

    while (param_num > 0)
    {
        int parameter = *params++;
        param_num--;

        if (parameter == 0)
        {
            // Reset
            con->fontCurPal = CONSOLE_WHITE;
            con->backgroundCurPal = CONSOLE_BLACK;
            con->sgrIntensity = 1;
            con->sgrForegroundIsRgb = 0;
            con->sgrBackgroundIsRgb = 0;
        }
        else if (parameter == 1)
        {
            // Bold or increased intensity
            con->sgrIntensity = 1;

            if ((con->sgrForegroundIsRgb == 0) && (con->fontCurPal < 8))
                con->fontCurPal += 8;
        }
        else if (parameter == 22)
        {
            // Normal intensity
            con->sgrIntensity = 0;

            if ((con->sgrForegroundIsRgb == 0) &&
                (con->fontCurPal >= 8) && (con->fontCurPal < 16))
            {
                con->fontCurPal -= 8;
            }
        }
        else if ((parameter >= 30) && (parameter <= 37))
        {
            // Set foreground color
            con->fontCurPal = parameter - 30;

            if (con->sgrIntensity)
                con->fontCurPal += 8;

            con->sgrForegroundIsRgb = false;
        }
        else if (parameter == 38)
        {
            // Set foreground color (256 colors, 24 bit color)

            if (param_num > 0)
            {
                int cmd = *params++;
                param_num--;

                if (cmd == 5) // 256 colors
                {
                    if (param_num > 0)
                    {
                        con->fontCurPal = *params++;
                        param_num--;

                        con->sgrForegroundIsRgb = false;
                    }
                }
                else if (cmd == 2) // 24 bit RGB
                {
                    if (param_num > 2)
                    {
                        con->fontCurRgb[0] = *params++;
                        con->fontCurRgb[1] = *params++;
                        con->fontCurRgb[2] = *params++;
                        param_num -= 3;

                        con->sgrForegroundIsRgb = true;
                    }
                }
            }
        }
        else if (parameter == 39)
        {
            // Default foreground color
            con->fontCurPal = CONSOLE_WHITE;
        }
        else if ((parameter >= 40) && (parameter <= 47))
        {
            // Set background color
            con->backgroundCurPal = parameter - 40;
            con->sgrBackgroundIsRgb = false;
        }
        else if (parameter == 48)
        {
            // Set background color (256 colors, 24 bit color)

            if (param_num > 0)
            {
                int cmd = *params++;
                param_num--;

                if (cmd == 5) // 256 colors
                {
                    if (param_num > 0)
                    {
                        con->backgroundCurPal = *params++;
                        param_num--;

                        con->sgrBackgroundIsRgb = false;
                    }
                }
                else if (cmd == 2) // 24 bit RGB
                {
                    if (param_num > 2)
                    {
                        con->backgroundCurRgb[0] = *params++;
                        con->backgroundCurRgb[1] = *params++;
                        con->backgroundCurRgb[2] = *params++;
                        param_num -= 3;

                        con->sgrBackgroundIsRgb = true;
                    }
                }
            }
        }
        else if (parameter == 49)
        {
            // Default background color
            con->backgroundCurPal = CONSOLE_BLACK;
            con->sgrBackgroundIsRgb = false;
        }
        else if ((parameter >= 90) && (parameter <= 97))
        {
            // Set bright foreground color
            con->fontCurPal = parameter - 90 + 8;
            con->sgrForegroundIsRgb = false;
        }
        else if ((parameter >= 100) && (parameter <= 107))
        {
            // Set bright background color
            con->backgroundCurPal = parameter - 100 + 8;
            con->sgrBackgroundIsRgb = false;
        }
    }
}

void consoleEnhancedColorHandler(PrintConsole *console)
{
    if (!console)
        console = currentConsole;

    console->HandleSgrCodes = console_handle_sgr;
}

static ssize_t con_write(const char *ptr, size_t len)
{
    const char *tmp = ptr;

    if (!tmp || len <= 0)
        return -1;

    size_t i = 0;
    size_t count = 0;

    while (i < len)
    {
        char chr = *(tmp++);
        i++;
        count++;

        if (chr == 0x1b && *tmp == '[')
        {
            bool escaping = true;
            const char *escapeseq = tmp;
            int escapelen = 0;
            unsigned int params[LIBNDS_CONSOLE_MAX_ANSI_PARAMS] = { 0 };
            unsigned int cur_param = 0;

            do
            {
                chr = *(tmp++);
                i++;
                count++;
                escapelen++;

                switch (chr)
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        params[cur_param] = (params[cur_param] * 10) + (chr - '0');
                        break;

                    case ';':
                        cur_param++;
                        if (cur_param == LIBNDS_CONSOLE_MAX_ANSI_PARAMS)
                            return -1;
                        break;

                    // Cursor directional movement
                    case 'A':
                    {
                        int new_y = currentConsole->cursorY - params[0];
                        if (new_y < 0)
                            new_y = 0;

                        currentConsole->cursorY = new_y;
                        escaping = false;
                        break;
                    }
                    case 'B':
                    {
                        int new_y = currentConsole->cursorY + params[0];
                        int max_y = currentConsole->windowHeight - 1;
                        if (new_y > max_y)
                            new_y = max_y;

                        currentConsole->cursorY = new_y;
                        escaping = false;
                        break;
                    }
                    case 'C':
                    {
                        int new_x = currentConsole->cursorX + params[0];
                        int max_x = currentConsole->windowWidth - 1;
                        if (new_x > max_x)
                            new_x = max_x;

                        currentConsole->cursorX = new_x;
                        escaping = false;
                        break;
                    }
                    case 'D':
                    {
                        int new_x = currentConsole->cursorX - params[0];
                        if (new_x < 0)
                            new_x = 0;

                        currentConsole->cursorX = new_x;
                        escaping = false;
                        break;
                    }

                    // Cursor position movement
                    case 'H':
                    case 'f':
                    {
                        int new_y = params[0];
                        int new_x = params[1];

                        int max_y = currentConsole->windowHeight - 1;
                        if (new_y > max_y)
                            new_y = max_y;

                        int max_x = currentConsole->windowWidth - 1;
                        if (new_x > max_x)
                            new_x = max_x;

                        currentConsole->cursorY = new_y;
                        currentConsole->cursorX = new_x;
                        escaping = false;
                        break;
                    }

                    // Screen clear
                    case 'J':
                        consoleCls(escapeseq[escapelen - 2]);
                        escaping = false;
                        break;

                    // Line clear
                    case 'K':
                        consoleClearLine(escapeseq[escapelen - 2]);
                        escaping = false;
                        break;

                    // Save cursor position
                    case 's':
                        currentConsole->prevCursorX = currentConsole->cursorX;
                        currentConsole->prevCursorY = currentConsole->cursorY;
                        escaping = false;
                        break;

                    // Load cursor position
                    case 'u':
                        currentConsole->cursorX = currentConsole->prevCursorX;
                        currentConsole->cursorY = currentConsole->prevCursorY;
                        escaping = false;
                        break;

                    // Color scan codes (list of numbers separated by ';')
                    case 'm':
                    {
                        if (currentConsole->HandleSgrCodes == NULL)
                        {
                            // Use the old buggy libnds SGR codes
                            console_handle_sgr_legacy(currentConsole,
                                                      cur_param + 1, params);
                        }
                        else
                        {
                            currentConsole->HandleSgrCodes(currentConsole,
                                                           cur_param + 1, params);
                        }

                        escaping = false;
                        break;
                    }
                }
            }
            while (escaping && (i < len));

            continue;
        }

        consolePrintChar(chr);
    }

    return count;
}

ConsoleOutFn libnds_stdout_write = NULL;
ConsoleOutFn libnds_stderr_write = NULL;

void consoleLoadFont(PrintConsole *console)
{
    // This function is only called if it is required to load console graphics,
    // so it makes no sense to call it unless there are graphics in the struct.
    sassert(console->font.gfx != NULL, "No font graphics found");

    u16 *palette = BG_PALETTE_SUB;

    // Check which display is being utilized
    if (console->fontBgGfx < BG_GFX_SUB)
        palette = BG_PALETTE;

    // Base pointer of the graphics slot
    u32 *bgGfxDestPtr = (u32 *)console->fontBgGfx;

    if (console->font.bpp == 1)
    {
        // The size of 1 BPP characters is the same as 4 BPP
        bgGfxDestPtr += console->fontCharOffset * (8 * 8) / (sizeof(u32) * 2);

        for (int i = 0; i < console->font.numChars * 8; i++)
        {
            u8 row = ((const u8 *)console->font.gfx)[i];
            u32 temp = 0;

            if (row & 0x01)
                temp |= 0xF;
            if (row & 0x02)
                temp |= 0xF0;
            if (row & 0x04)
                temp |= 0xF00;
            if (row & 0x08)
                temp |= 0xF000;
            if (row & 0x10)
                temp |= 0xF0000;
            if (row & 0x20)
                temp |= 0xF00000;
            if (row & 0x40)
                temp |= 0xF000000;
            if (row & 0x80)
                temp |= 0xF0000000;

            bgGfxDestPtr[i] = temp;
        }
    }
    else if (console->font.bpp == 4)
    {
        bgGfxDestPtr += console->fontCharOffset * (8 * 8) / (sizeof(u32) * 2);

        size_t size = console->font.numChars * (8 * 8) / 2;
        DC_FlushRange(console->font.gfx, size);
        dmaCopy(console->font.gfx, bgGfxDestPtr, size);
    }
    else if (console->font.bpp == 8)
    {
        bgGfxDestPtr += console->fontCharOffset * (8 * 8) / sizeof(u32);

        size_t size = console->font.numChars * (8 * 8);
        DC_FlushRange(console->font.gfx, size);
        dmaCopy(console->font.gfx, bgGfxDestPtr, size);

        // TODO: Extended palettes aren't supported currently
        sassert(console->fontPalIndex == 0, "Extended palettes not supported");
        console->fontCurPal = 0;
    }

    // Palette graphics are optional. 1 BPP and 4 BPP fonts can use the 16
    // default palettes loaded by this function.
    if (console->font.pal)
    {
        // Use user-provided palette
        size_t size = console->font.numColors * 2;
        DC_FlushRange(console->font.pal, size);
        dmaCopy(console->font.pal, palette + (console->fontPalIndex * 16), size);

        // Set the user-provided palette as the active one
        console->fontCurPal = console->fontPalIndex;
    }
    else
    {
        // Set default palette (4bpp and 8bpp variants)
        palette[0] = RGB15(0, 0, 0); // Clear color (displayed under BG layers)
        palette[16 * 16 - 1] = RGB15(31, 31, 31); // 47 & 39 bright white

        if (console->font.bpp <= 4)
        {
            palette[1 * 16 - 1] = RGB15(0, 0, 0);   // 30 normal black
            palette[2 * 16 - 1] = RGB15(15, 0, 0);  // 31 normal red
            palette[3 * 16 - 1] = RGB15(0, 15, 0);  // 32 normal green
            palette[4 * 16 - 1] = RGB15(15, 15, 0); // 33 normal yellow

            palette[5 * 16 - 1] = RGB15(0, 0, 15);   // 34 normal blue
            palette[6 * 16 - 1] = RGB15(15, 0, 15);  // 35 normal magenta
            palette[7 * 16 - 1] = RGB15(0, 15, 15);  // 36 normal cyan
            palette[8 * 16 - 1] = RGB15(24, 24, 24); // 37 normal white

            palette[9 * 16 - 1] = RGB15(15, 15, 15); // 40 bright black
            palette[10 * 16 - 1] = RGB15(31, 0, 0);  // 41 bright red
            palette[11 * 16 - 1] = RGB15(0, 31, 0);  // 42 bright green
            palette[12 * 16 - 1] = RGB15(31, 31, 0); // 43 bright yellow

            palette[13 * 16 - 1] = RGB15(0, 0, 31);   // 44 bright blue
            palette[14 * 16 - 1] = RGB15(31, 0, 31);  // 45 bright magenta
            palette[15 * 16 - 1] = RGB15(0, 31, 31);  // 46 bright cyan

            // Set the white pre-defined palette as the active palette
            console->fontCurPal = 15;
        }
    }

    PrintConsole *tmp = consoleSelect(console);
    consoleCls('2');
    consoleSelect(tmp);
}

PrintConsole *consoleInitEx(PrintConsole *console, int layer, BgType type, BgSize size,
                            int mapBase, int tileBase, int palIndex, int fontCharOffset,
                            bool mainDisplay, bool loadGraphics)
{
    static bool firstConsoleInit = true;

    if (firstConsoleInit)
    {
        libnds_stdout_write = con_write;
        libnds_stderr_write = con_write;

        firstConsoleInit = false;
    }

    if (console)
        currentConsole = console;
    else
        console = currentConsole;

    *currentConsole = defaultConsole;

    if (mainDisplay)
        console->bgId = bgInit(layer, type, size, mapBase, tileBase);
    else
        console->bgId = bgInitSub(layer, type, size, mapBase, tileBase);

    console->fontBgGfx = bgGetGfxPtr(console->bgId);
    console->fontBgMap = bgGetMapPtr(console->bgId);
    console->fontCharOffset = fontCharOffset;
    console->fontPalIndex = palIndex;

    console->fontCurPal = 0;
    console->backgroundCurPal = 0;
    console->sgrIntensity = 1; // High
    console->sgrForegroundIsRgb = 0;
    console->sgrBackgroundIsRgb = 0;

    if (loadGraphics)
    {
        consoleLoadFont(console);
        consoleCls('2');
    }

    console->prevCursorX = 0;
    console->prevCursorY = 0;

    return currentConsole;
}

PrintConsole *consoleInit(PrintConsole *console, int layer, BgType type, BgSize size,
                          int mapBase, int tileBase, bool mainDisplay, bool loadGraphics)
{
    return consoleInitEx(console, layer, type, size, mapBase, tileBase, 0, 0,
                         mainDisplay, loadGraphics);
}

PrintConsole *consoleSelect(PrintConsole *console)
{
    // Make sure that the buffers for the current console are flushed before
    // switching consoles.
    fflush(stdout);
    fflush(stderr);

    PrintConsole *tmp = currentConsole;
    currentConsole = console;
    return tmp;
}

void consoleSetFont(PrintConsole *console, ConsoleFont *font)
{
    if (!console)
        console = currentConsole;

    console->font = *font;

    consoleLoadFont(console);
}

void consoleDebugInit(DebugDevice device)
{
    switch (device)
    {
        case DebugDevice_NOCASH:
            libnds_stderr_write = nocash_write;
            break;
        case DebugDevice_CONSOLE:
            libnds_stderr_write = con_write;
            break;
        case DebugDevice_NULL:
            libnds_stderr_write = NULL;
            break;
    }
}

// Places the console in a default mode using bg0 of the sub display, and VRAM_C
// for font and map. This is provided for rapid prototyping and nothing more.
PrintConsole *consoleDemoInit(void)
{
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    setBrightness(2, 0);

    return consoleInit(NULL, DEFAULT_CONSOLE_BG_LAYER, BgType_Text4bpp, BgSize_T_256x256,
                       DEFAULT_CONSOLE_MAP_BASE, DEFAULT_CONSOLE_GFX_BASE, false, true);
}

static void newRow(void)
{
    currentConsole->cursorY++;

    if (currentConsole->cursorY >= currentConsole->windowHeight)
    {
        int rowCount;
        int colCount;

        currentConsole->cursorY--;

        for (rowCount = 0; rowCount < currentConsole->windowHeight - 1; rowCount++)
        {
            for (colCount = 0; colCount < currentConsole->windowWidth; colCount++)
            {
                int dst_index = (colCount + currentConsole->windowX)
                              + (rowCount + currentConsole->windowY) * currentConsole->consoleWidth;

                int src_index = (colCount + currentConsole->windowX)
                              + (rowCount + currentConsole->windowY + 1)
                                 * currentConsole->consoleWidth;

                u16 value = currentConsole->fontBgMap[src_index];

                currentConsole->fontBgMap[dst_index] = value;
            }
        }

        for (colCount = 0; colCount < currentConsole->windowWidth; colCount++)
        {
            int index = (colCount + currentConsole->windowX)
                      + (rowCount + currentConsole->windowY) * currentConsole->consoleWidth;

            u16 value = ' ' + currentConsole->fontCharOffset - currentConsole->font.asciiOffset;

            currentConsole->fontBgMap[index] = value;
        }
    }
}

void consolePrintChar(char c)
{
    if (c == '\0')
        return;

    if (currentConsole->PrintChar)
    {
        if (currentConsole->PrintChar(currentConsole, c))
            return;
    }

    if (currentConsole->fontBgMap == NULL)
        return;

    if (currentConsole->cursorX >= currentConsole->windowWidth)
    {
        currentConsole->cursorX = 0;

        newRow();
    }

    switch (c)
    {
        // The only special characters we will handle are tab (\t), carriage
        // return (\r), line feed (\n) and backspace (\b).
        //
        // Line feed goes to next line and puts the cursor at the beginning.
        // Carriage return only puts the cursor at the beginning. For everything
        // else, use VT sequences.
        //
        // Reason: VT sequences are more specific to the task of cursor
        // placement.  The special escape sequences \b \f & \v are archaic and
        // non-portable.

        case '\b':
        {
            currentConsole->cursorX--;

            if (currentConsole->cursorX < 0)
            {
                if (currentConsole->cursorY > 0)
                {
                    currentConsole->cursorX = currentConsole->windowX - 1;
                    currentConsole->cursorY--;
                }
                else
                {
                    currentConsole->cursorX = 0;
                }
            }

            uint16_t tile = ' ' + currentConsole->fontCharOffset - currentConsole->font.asciiOffset;

            int index = currentConsole->cursorX + currentConsole->windowX
                      + (currentConsole->cursorY + currentConsole->windowY)
                         * currentConsole->consoleWidth;

            currentConsole->fontBgMap[index] = TILE_PALETTE(currentConsole->fontCurPal) | tile;
            break;
        }
        case '\t':
        {
            int spaces = currentConsole->tabSize
                       - ((currentConsole->cursorX) % (currentConsole->tabSize));

            currentConsole->cursorX += spaces;
            break;
        }
        case '\n':
            newRow();
            // fallthrough
        case '\r':
            currentConsole->cursorX = 0;
            break;

        default:
        {
            if (c < currentConsole->font.asciiOffset)
                c = ' ';
            if (c >= currentConsole->font.asciiOffset + currentConsole->font.numChars)
                c = ' ';

            uint16_t tile = c + currentConsole->fontCharOffset - currentConsole->font.asciiOffset;

            int index = currentConsole->cursorX + currentConsole->windowX
                      + (currentConsole->cursorY + currentConsole->windowY)
                         * currentConsole->consoleWidth;

            currentConsole->fontBgMap[index] = TILE_PALETTE(currentConsole->fontCurPal) | tile;
            currentConsole->cursorX++;
            break;
        }
    }
}

void consoleClear(void)
{
    consoleCls('2');
}

void consoleSetCursor(PrintConsole *console, int x, int y)
{
    if (!console)
        console = currentConsole;

    int max_y = console->windowHeight - 1;
    if (y > max_y)
        y = max_y;

    int max_x = console->windowWidth - 1;
    if (x > max_x)
        x = max_x;

    console->cursorX = x;
    console->cursorY = y;
}

void consoleAddToCursor(PrintConsole *console, int deltaX, int deltaY)
{
    if (!console)
        console = currentConsole;

    int x = console->cursorX + deltaX;
    int y = console->cursorY + deltaY;

    int max_y = console->windowHeight - 1;
    if (y > max_y)
        y = max_y;
    if (y < 0)
        y = 0;

    int max_x = console->windowWidth - 1;
    if (x > max_x)
        x = max_x;
    if (x < 0)
        x = 0;

    console->cursorX = x;
    console->cursorY = y;
}

void consoleGetCursor(PrintConsole *console, int *x, int *y)
{
    if (!console)
        console = currentConsole;

    if (x != NULL)
        *x = console->cursorX;

    if (y != NULL)
        *y = console->cursorY;
}

void consoleSetColor(PrintConsole *console, ConsoleColor color)
{
    if (!console)
        console = currentConsole;

    // Only colors 0 to 15 are allowed, treat the rest as white
    if (color >= CONSOLE_DEFAULT)
        console->fontCurPal = CONSOLE_WHITE;
    else
        console->fontCurPal = color;
}

void consoleSetBackgroundColor(PrintConsole *console, ConsoleColor color)
{
    if (!console)
        console = currentConsole;

    // Only colors 0 to 15 are allowed, treat the rest as black
    if (color >= CONSOLE_DEFAULT)
        console->backgroundCurPal = CONSOLE_BLACK;
    else
        console->backgroundCurPal = color;
}

void consoleSetWindow(PrintConsole *console, int x, int y, int width, int height)
{
    if (!console)
        console = currentConsole;

    console->windowWidth = width;
    console->windowHeight = height;
    console->windowX = x;
    console->windowY = y;

    console->cursorX = 0;
    console->cursorY = 0;
}

void consoleSetCustomStdout(ConsoleOutFn fn)
{
    if (fn != NULL)
        libnds_stdout_write = fn;
    else
        libnds_stdout_write = con_write;
}

void consoleSetCustomStderr(ConsoleOutFn fn)
{
    if (fn != NULL)
        libnds_stderr_write = fn;
    else
        libnds_stderr_write = con_write;
}

// ---------------------------------------------------------------------------

static ConsoleArm7Ipc *arm7con = NULL;
static PrintConsole *arm7console = NULL;

int consoleArm7Setup(PrintConsole *console, size_t buffer_size)
{
    if (buffer_size == 0)
        return -1;

    // Fail if the console has already been initalized
    if (arm7con != NULL)
        return -2;

    size_t total_size = sizeof(ConsoleArm7Ipc) + buffer_size;

    ConsoleArm7Ipc *newcon = malloc(total_size);
    if (newcon == NULL)
        return -3;

    newcon->buffer_size = buffer_size;
    newcon->read_index = 0;
    newcon->write_index = 0;

    // Flush the buffer and store the unchached version of the pointer because
    // we don't want to manage the cache all the time when reading from here.

    DC_FlushRange(newcon, total_size);

    arm7con = memUncached(newcon);

    // Send the pointer to the ARM7

    FifoMessage msg;

    msg.type = SYS_SET_ARM7_CONSOLE;
    msg.setArm7Console.buffer = newcon;

    fifoSendDatamsg(FIFO_SYSTEM, sizeof(msg), (u8 *)&msg);

    arm7console = console;

    return 0;
}

int consoleArm7Disable(void)
{
    if (arm7con == NULL)
        return -1;

    FifoMessage msg;

    msg.type = SYS_SET_ARM7_CONSOLE;
    msg.setArm7Console.buffer = NULL;

    fifoSendDatamsg(FIFO_SYSTEM, sizeof(msg), (u8 *)&msg);

    free(memCached(arm7con));
    arm7con = NULL;

    arm7console = NULL;

    return 0;
}

void consoleArm7Flush(void)
{
    if (arm7con == NULL)
        return;

    // Preserve currently-selected console
    PrintConsole *old_con = consoleSelect(arm7console);

    while (arm7con->read_index != arm7con->write_index)
    {
        char c = arm7con->buffer[arm7con->read_index];

        arm7con->read_index++;
        if (arm7con->read_index == arm7con->buffer_size)
            arm7con->read_index = 0;

        consolePrintChar(c);
    }

    // Restore previous console
    consoleSelect(old_con);
}
