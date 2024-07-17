// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Antonio Niño Díaz

#include <nds/arm9/console.h>
#include <nds/interrupts.h>

static void libndsCrashPuts(const char *message)
{
    while (1)
    {
        char c = *message++;
        if (c == '\0')
            break;
        consolePrintChar(c);
    }
}

__attribute__((noreturn)) void libndsCrash(const char *message)
{
    REG_IME = 0;

    consoleDemoInit();

    libndsCrashPuts("libnds fatal error:\n\n");
    libndsCrashPuts(message);

    while (1)
        swiWaitForVBlank();
}
