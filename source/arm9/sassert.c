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

__attribute__((noreturn))
void __sassert(const char *fileName, int lineNumber, const char *conditionString,
               const char *format, ...)
{
    va_list ap;

    consoleDemoInit();

    consoleSetColor(NULL, CONSOLE_LIGHT_YELLOW);

    printf("Assertion!\n\n");

    consoleSetColor(NULL, CONSOLE_DEFAULT);

    printf("File:\n"
           "%s\n\n"                 // Print filename
           "Line: %d\n\n"           // Print line number
           "Condition:\n"
           "%s\n\n"                 // Print condition message
           "Message:\n",
           fileName, lineNumber, conditionString);

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);

    consoleSetCursor(NULL, 0, 23);
    printf("Press SELECT+START to exit");

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
    printf("Failed to exit            ");

    // Return an error code to the loader
    exit(-1);

    while (1)
        swiWaitForVBlank();
}
