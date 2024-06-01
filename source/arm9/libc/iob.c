// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz
// Copyright (c) 2023 Adrian "asie" Siekierka

#include <stdio.h>

#include <nds/arm9/keyboard.h>
#include <nds/arm9/input.h>
#include <nds/cothread.h>
#include <nds/interrupts.h>

#include "arm9/libnds_internal.h"

// Newline buffer so that we can support pressing the Backspace key.
// If not defined, unbuffered keyboard input is used.
#define INPUT_BUFFER_SIZE 128
#ifdef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_MASK (INPUT_BUFFER_SIZE - 1)
static char stdin_buf[INPUT_BUFFER_SIZE];
static uint16_t stdin_buf_left = 0;
static uint16_t stdin_buf_right = 0;
#endif
bool stdin_buf_empty = false;
// Buffers so that we can send to the console full ANSI escape sequences.
#define OUTPUT_BUFFER_SIZE 16
static char stdout_buf[OUTPUT_BUFFER_SIZE + 1];
static char stderr_buf[OUTPUT_BUFFER_SIZE + 1];
static uint16_t stdout_buf_len = 0;
static uint16_t stderr_buf_len = 0;

static int putc_buffered(char c, char *buf, uint16_t *buf_len, ConsoleOutFn fn)
{
    if ((c == 0x1B) || (*buf_len > 0))
    {
        buf[*buf_len] = c;
        (*buf_len)++;
        buf[*buf_len] = 0;

        if ((*buf_len == OUTPUT_BUFFER_SIZE) || (c == '\n') || (c == '\r') ||
            ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')))
        {
            fn(buf, *buf_len);
            *buf_len = 0;
        }
    }
    else
    {
        fn(&c, 1);
    }

    return c;
}

static int stderr_putc_buffered(char c, FILE *file)
{
    (void)file;

    if (libnds_stderr_write == NULL)
        return c;

    return putc_buffered(c, stderr_buf, &stderr_buf_len, libnds_stderr_write);
}

static int stdout_putc_buffered(char c, FILE *file)
{
    (void)file;

    // If stdout is not initialized, the user may have nonetheless initialized
    // a debug console with consoleDebugInit(). Try to fall back to that.
    if (libnds_stdout_write == NULL)
        return stderr_putc_buffered(c, file);

    return putc_buffered(c, stdout_buf, &stdout_buf_len, libnds_stdout_write);
}

static int stdin_getc_keyboard(FILE *file)
{
    (void)file;

    static int shown = 0;
    int c = -1;

#ifdef INPUT_BUFFER_SIZE
    if (shown == 0 && stdin_buf_left != stdin_buf_right)
    {
        c = stdin_buf[stdin_buf_left];
        stdin_buf_left = (stdin_buf_left + 1) & INPUT_BUFFER_MASK;
        return c;
    }
#endif

    if (shown == 0)
    {
        keyboardShow();
        shown = 1;
    }

    while (true)
    {
        scanKeys();
#ifdef INPUT_BUFFER_SIZE
        stdin_buf_empty = stdin_buf_left == stdin_buf_right;
        int kc = keyboardUpdate();
        stdin_buf_empty = false;
        if (kc == DVK_BACKSPACE)
        {
            if (stdin_buf_left != stdin_buf_right)
            {
                stdin_buf_right = (stdin_buf_right - 1) & INPUT_BUFFER_MASK;
            }
        }
        else if (kc != -1)
        {
            uint16_t next_right = (stdin_buf_right + 1) & INPUT_BUFFER_MASK;
            // if about to overflow buffer, pop char
            // if newline, finish writing string - hide keyboard + pop char
            if (next_right == stdin_buf_left || kc == '\n')
            {
                if (kc == '\n')
                {
                    keyboardHide();
                    shown = 0;
                }

                c = stdin_buf[stdin_buf_left];
                stdin_buf_left = (stdin_buf_left + 1) & INPUT_BUFFER_MASK;
            }
            stdin_buf[stdin_buf_right] = kc;
            stdin_buf_right = next_right;
        }
#else
        c = keyboardUpdate();
#endif
        if (c != -1)
            break;
        cothread_yield_irq(IRQ_VBLANK);
    }

#ifndef INPUT_BUFFER_SIZE
    if (c == '\n')
    {
        keyboardHide();
        shown = 0;
    }
#endif

    return c;
}

static FILE __stdin = FDEV_SETUP_STREAM(NULL, stdin_getc_keyboard, NULL,
                                        _FDEV_SETUP_READ);
static FILE __stdout = FDEV_SETUP_STREAM(stdout_putc_buffered, NULL, NULL,
                                         _FDEV_SETUP_WRITE);
static FILE __stderr = FDEV_SETUP_STREAM(stderr_putc_buffered, NULL, NULL,
                                         _FDEV_SETUP_WRITE);

FILE *const stdin = &__stdin;
FILE *const stdout = &__stdout;
FILE *const stderr = &__stderr;
