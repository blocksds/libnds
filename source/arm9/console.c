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

#include <nds/arm9/background.h>
#include <nds/arm9/console.h>
#include <nds/arm9/video.h>
#include <nds/debug.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>

#include "common/libnds_internal.h"
#include "default_font.h"

PrintConsole defaultConsole =
{
    // Font:
    {
        (u16 *)default_fontTiles, // font gfx
        0,                        // font palette
        0,                        // font color count
        1,                        // bpp
        32,                       // first ascii character in the set
        96                        // number of characters in the font set
    },
    0,  // font background map
    0,  // font background gfx
    22, // map base
    3,  // char base
    0,  // bg layer in use
    -1, // bg id
    0,
    0, // cursorX cursorY
    0,
    0,     // prevcursorX prevcursorY
    32,    // console width
    24,    // console height
    0,     // window x
    0,     // window y
    32,    // window width
    24,    // window height
    3,     // tab size
    0,     // font character offset
    0,     // selected palette
    0,     // print callback
    false, // console initialized
    true,  // load graphics
};

PrintConsole currentCopy;

PrintConsole *currentConsole = &currentCopy;

PrintConsole *consoleGetDefault(void)
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

            while (i++ < ((currentConsole->windowWidth - colTemp) - 2))
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
            unsigned int params[2] = { 0 };
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
                        if (cur_param == 2) // Only one ';' supported
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
                        int new_x = currentConsole->cursorY - params[0];
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

                    // Color scan codes
                    case 'm':
                    {
                        int parameter = params[0];
                        int intensity = params[1];

                        // only handle 30-37,39 and intensity for the color changes
                        parameter -= 30;

                        // 39 is the reset code
                        if (parameter == 9)
                            parameter = 15;
                        else if (parameter > 8)
                            parameter -= 2;
                        else if (intensity)
                            parameter += 8;

                        if (parameter < 16 && parameter >= 0)
                            currentConsole->fontCurPal = parameter << 12;

                        escaping = false;
                        break;
                    }
                }
            } while (escaping);
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
    u16 *palette = BG_PALETTE_SUB;

    // Check which display is being utilized
    if (console->fontBgGfx < BG_GFX_SUB)
        palette = BG_PALETTE;

    console->fontCurPal <<= 12;

    if (console->font.bpp == 1)
    {
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

            ((u32 *)console->fontBgGfx)[i] = temp;
        }
    }
    else if (console->font.bpp == 4)
    {
        if (console->font.gfx)
            dmaCopy(console->font.gfx, console->fontBgGfx,
                    console->font.numChars * 64 / 2);
    }
    else if (console->font.bpp == 8)
    {
        if (console->font.gfx)
            dmaCopy(console->font.gfx, console->fontBgGfx,
                    console->font.numChars * 64);

        console->fontCurPal = 0;
    }

    if (console->font.pal)
    {
        // Use user-provided palette
        dmaCopy(console->font.pal, palette + (console->fontCurPal >> 8),
                    console->font.numColors * 2);
    }
    else
    {
        // Set default palette (4bpp and 8bpp variants)
        palette[0] = RGB15(0, 0, 0);
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

            console->fontCurPal = 15 << 12;
        }
    }

    PrintConsole *tmp = consoleSelect(console);
    consoleCls('2');
    consoleSelect(tmp);
}

PrintConsole *consoleInit(PrintConsole *console, int layer, BgType type, BgSize size,
                          int mapBase, int tileBase, bool mainDisplay, bool loadGraphics)
{
    static bool firstConsoleInit = true;

    if (firstConsoleInit)
    {
        libnds_stdout_write = con_write;
        libnds_stderr_write = con_write;

        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);

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

    console->fontBgGfx = (u16 *)bgGetGfxPtr(console->bgId);
    console->fontBgMap = (u16 *)bgGetMapPtr(console->bgId);

    console->consoleInitialised = 1;

    consoleCls('2');

    if (loadGraphics)
        consoleLoadFont(console);

    return currentConsole;
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
    int buffertype = _IONBF;

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
    setvbuf(stderr, NULL, buffertype, 0);
}

// Places the console in a default mode using bg0 of the sub display, and VRAM_C
// for font and map. This is provided for rapid prototyping and nothing more.
PrintConsole *consoleDemoInit(void)
{
    videoSetModeSub(MODE_0_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    setBrightness(2, 0);

    return consoleInit(NULL, defaultConsole.bgLayer, BgType_Text4bpp, BgSize_T_256x256,
                       defaultConsole.mapBase, defaultConsole.gfxBase, false, true);
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
                int dst_index =
                    (colCount + currentConsole->windowX)
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
            int index =
                (colCount + currentConsole->windowX)
                + (rowCount + currentConsole->windowY) * currentConsole->consoleWidth;

            u16 value =
                ' ' + currentConsole->fontCharOffset - currentConsole->font.asciiOffset;

            currentConsole->fontBgMap[index] = value;
        }
    }
}

void consolePrintChar(char c)
{
    if (c == 0)
        return;
    if (currentConsole->fontBgMap == 0)
        return;

    if (currentConsole->PrintChar)
        if (currentConsole->PrintChar(currentConsole, c))
            return;

    if (currentConsole->cursorX >= currentConsole->windowWidth)
    {
        currentConsole->cursorX = 0;

        newRow();
    }

    switch (c)
    {
        // The only special characters we will handle are tab (\t), carriage
        // return (\r), line feed (\n) and backspace (\b). Carriage return &
        // line feed will function the same: go to next line and put cursor at
        // the beginning. For everything else, use VT sequences.
        //
        // Reason: VT sequences are more specific to the task of cursor
        // placement.  The special escape sequences \b \f & \v are archaic and
        // non-portable.

        case 8:
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

            currentConsole
                ->fontBgMap[currentConsole->cursorX + currentConsole->windowX
                            + (currentConsole->cursorY + currentConsole->windowY)
                                  * currentConsole->consoleWidth] =
                currentConsole->fontCurPal
                | (u16)(' ' + currentConsole->fontCharOffset
                        - currentConsole->font.asciiOffset);

            break;

        case 9:
            currentConsole->cursorX +=
                currentConsole->tabSize
                - ((currentConsole->cursorX) % (currentConsole->tabSize));
            break;

        case 10:
            newRow();
            // fallthrough
        case 13:
            currentConsole->cursorX = 0;
            break;

        default:
            currentConsole
                ->fontBgMap[currentConsole->cursorX + currentConsole->windowX
                            + (currentConsole->cursorY + currentConsole->windowY)
                                  * currentConsole->consoleWidth] =
                currentConsole->fontCurPal
                | (u16)(c + currentConsole->fontCharOffset
                        - currentConsole->font.asciiOffset);
            ++currentConsole->cursorX;
            break;
    }
}

void consoleClear(void)
{
    consoleCls('2');
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
