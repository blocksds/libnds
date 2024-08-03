// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Antonio Niño Díaz

#include <stdio.h>

#include <nds/arm9/sassert.h>

// This is called by assert() from picolibc
__attribute__((noreturn))
void __assert_func(const char *file, int line, const char *func,
                   const char *failedexpr)
{
    __sassert(file, line, failedexpr, "Function: %s", func);
}
