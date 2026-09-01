// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2013 Jason Rogers (dovoto)
// Copyright (C) 2013 Michael Theall (mtheall)
// Copyright (C) 2023 Antonio Niño Díaz

// Simple routine to display assertion failure messages.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <nds/arm9/console.h>
#include <nds/arm9/input.h>
#include <nds/input.h>
#include <nds/interrupts.h>

static void assert_print_prelude(const char *fileName, int lineNumber,
                                const char *conditionString)
{
    consoleDemoInit();

    consoleSetColor(NULL, CONSOLE_LIGHT_YELLOW);

    consolePrintString("Assertion!\n\n");

    consoleSetColor(NULL, CONSOLE_DEFAULT);

    consolePrintString("File:\n");
    consolePrintString(fileName);
    consolePrintString("\n\n"
                       "Line:\n");
    consolePrintUnsigned(lineNumber, 10);
    consolePrintString("\n\n"
                       "Condition:\n");
    consolePrintString(conditionString);
    consolePrintString("\n\n"
                       "Message:\n");
}

LIBNDS_NORETURN
static void assert_print_epilogue(void)
{
    consoleSetCursor(NULL, 0, 23);
    consolePrintString("Press SELECT+START to exit");

    while (1)
    {
        swiWaitForVBlank();
        scanKeys();
        uint16_t keys_mask = KEY_START | KEY_SELECT;
        if ((keysHeld() & keys_mask) == keys_mask)
            break;
    }

    // Print an error message over the previous message. This shouldn't normally
    // be seen by the user because exit() is called right afterwards. It will
    // only be seen if exit() hangs, which is not its normal behaviour (it
    // should power off the NDS if it fails to exit to the loader).
    consoleSetCursor(NULL, 0, 23);
    consolePrintString("Failed to exit            ");

    // Return an error code to the loader
    exit(-1);

    while (1)
        swiWaitForVBlank();
}

LIBNDS_NORETURN
void __sassert_nofmt(const char *fileName, int lineNumber, const char *conditionString,
                     const char *format)
{
    // This function can be called from IRQ handlers

    assert_print_prelude(fileName, lineNumber, conditionString);

    consolePrintString(format);

    assert_print_epilogue();
}

LIBNDS_NORETURN
void __sassert(const char *fileName, int lineNumber, const char *conditionString,
               const char *format, ...)
{
    // This function can't be called from IRQ handlers, so it's possible to use
    // the printf() family of functions.

    va_list ap;

    assert_print_prelude(fileName, lineNumber, conditionString);

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);

    assert_print_epilogue();
}
