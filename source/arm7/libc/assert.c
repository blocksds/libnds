// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Antonio Niño Díaz

#include <stdio.h>

#include <nds.h>

// This is called by assert() from picolibc
__attribute__((noreturn))
void __assert_func(const char *file, int line, const char *func,
                   const char *failedexpr)
{
    REG_IME = 0;

    // The console is setup to redirect stderr to the no$gba debug console by
    // default.
    fprintf(stderr,
           "ARM7 assertion failed!\n"
           "File: %s\n"
           "Line: %d\n"
           "Condition: %s\n"
           "Function: %s\n",
           file, line, failedexpr, func);

    while (1);
}
