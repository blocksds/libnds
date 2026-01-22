// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Antonio Niño Díaz

// This console is designed to be small, that's why many functions have been
// marked as "noinline". This will have a small cost in speed, but a big gain in
// code size.

#include <stdarg.h>
#include <stdio.h>

#include <nds/arm7/console.h>
#include <nds/bios.h>
#include <nds/fifocommon.h>

#include "common/libnds_internal.h"

static ConsoleArm7Ipc *con = NULL;

// This is an internal libnds function called by the FIFO handler when the ARM9
// sets up the ARM7 console system.
int consoleSetup(ConsoleArm7Ipc *c)
{
    con = c;

    return 0;
}

bool consoleIsSetup(void)
{
    if (con == NULL)
        return false;

    return true;
}

static uint16_t consoleNextWriteIndex(ConsoleArm7Ipc *c)
{
    if (c->write_index + 1 >= c->buffer_size)
        return 0;
    else
        return c->write_index + 1;
}

LIBNDS_NOINLINE bool consoleIsFull(void)
{
    uint16_t next_write_index = consoleNextWriteIndex(con);

    if (next_write_index == con->read_index)
        return true;

    return false;
}

LIBNDS_NOINLINE int consolePrintChar(char c)
{
    if (con == NULL)
        return -1;

    if (consoleIsFull())
    {
        consoleFlush();

        do
        {
            // Give some time to the ARM9 to print more than one character so
            // that we don't send too many FIFO commands. It's a lot faster to
            // add characters from the ARM7 than to print them from the ARM9.
            swiDelay(100);
        }
        while (consoleIsFull());
    }

    con->buffer[con->write_index] = c;
    con->write_index++;

    if (con->write_index == con->buffer_size)
        con->write_index = 0;

    return 0;
}

void consoleFlush(void)
{
    fifoSendValue32(FIFO_SYSTEM, SYS_ARM7_CONSOLE_FLUSH);
}

LIBNDS_NOINLINE void consolePuts(const char *str)
{
    while (*str != '\0')
        consolePrintChar(*str++);
}

LIBNDS_NOINLINE void consolePrintNumUnsigned(uint32_t num, uint32_t base)
{
    // When printing the number we actually get the digits in reverse, so we
    // need a small buffer to store the number and then print it in reverse from
    // there. UINT32_MAX (4 294 967 295) fits in this buffer:
    char tmp[11];

    static const char digits_str[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'
    };

    int len = 0;

    while (1)
    {
        uint32_t digit = num % base;
        num = num / base;

        tmp[len++] = digits_str[digit];

        // Check this at the end of the first iteration so that the number "0"
        // can be printed instead of not printing anything.
        if (num == 0)
            break;
    }

    while (len > 0)
    {
        len--;
        consolePrintChar(tmp[len]);
    }
}

int consoleVprintf(const char *fmt, va_list args)
{
    while (1)
    {
        char c = *fmt++;
        if (c == '\0')
            break;

        if (c != '%')
        {
            consolePrintChar(c);
        }
        else
        {
            // We don't support modifiers so we only need to check one more
            // character.

            c = *fmt++;

            switch (c)
            {
                case '%':
                {
                    consolePrintChar('%');
                    break;
                }
                case 'c':
                {
                    int val = va_arg(args, int);
                    consolePrintChar(val);
                    break;
                }
                case 's':
                {
                    const char *str = va_arg(args, char *);
                    consolePuts(str);
                    break;
                }
                case 'x':
                {
                    unsigned int unum = va_arg(args, unsigned int);
                    consolePrintNumUnsigned(unum, 16);
                    break;
                }
                case 'u':
                {
                    unsigned int unum = va_arg(args, unsigned int);
                    consolePrintNumUnsigned(unum, 10);
                    break;
                }
                case 'd':
                {
                    int num = va_arg(args, int);
                    unsigned int unum;

                    if (num < 0)
                    {
                        consolePrintChar('-');
                        unum = -num;
                    }
                    else
                    {
                        unum = num;
                    }

                    consolePrintNumUnsigned(unum, 10);
                    break;
                }
                default:
                {
                    return -1;
                }
            }
        }
    }

    return 0;
}

int consolePrintf(const char *fmt, ...)
{
    int ret;
    va_list va;

    va_start(va, fmt);
    ret = consoleVprintf(fmt, va);
    va_end(va);

    return ret;
}
