// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <stdio.h>

#include <nds/debug.h>

#include "common/libnds_internal.h"

#define NOCASHGBA_BUFFER_SIZE 120
static char nocash_buf[NOCASHGBA_BUFFER_SIZE];
static uint8_t nocash_buf_len = 0;

int nocash_putc_buffered(char c, FILE *file)
{
    (void)file;

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
