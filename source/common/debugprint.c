// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2025 Antonio Niño Díaz

#include <stdio.h>

#include <nds/debug.h>

#include "common/libnds_internal.h"

// TODO: melonDS doesn't support the debug registers on the ARM7 for now. When
// it supports them we can switch both CPUs to the ARM9 version of this
// function.

#ifdef ARM9
#define NOCASHGBA_BUFFER_SIZE 120
static char nocash_buf[NOCASHGBA_BUFFER_SIZE + 1]; // Leave space for terminator
static uint8_t nocash_buf_len = 0;

int nocash_putc_buffered(char c, FILE *file)
{
    (void)file;

    nocash_buf[nocash_buf_len] = c;
    nocash_buf_len++;

    if ((c == '\n') || (nocash_buf_len == NOCASHGBA_BUFFER_SIZE))
    {
        nocash_buf[nocash_buf_len] = '\0';
        REG_NOCASH_STR_PARAM = (uintptr_t)&nocash_buf;
        nocash_buf_len = 0;
    }

    return c;
}
#endif // ARM9

#ifdef ARM7
#define NOCASHGBA_BUFFER_SIZE 120
static char nocash_buf[NOCASHGBA_BUFFER_SIZE];
static uint8_t nocash_buf_len = 0;

int nocash_putc_buffered(char c, FILE *file)
{
    (void)file;

    // Don't send '\n' characters to nocashWrite(), it adds them by default.
    if (c != '\n')
    {
        nocash_buf[nocash_buf_len] = c;
        nocash_buf_len++;
    }

    if ((c == '\n') || (nocash_buf_len == NOCASHGBA_BUFFER_SIZE))
    {
        nocashWrite(nocash_buf, nocash_buf_len);
        nocash_buf_len = 0;
    }

    return c;
}
#endif
