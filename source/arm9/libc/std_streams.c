// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz
// Copyright (C) 2023 Adrian "asie" Siekierka

#include <stdio.h>
#include <stdio-bufio.h>
#include <unistd.h>

#include "arm9/libnds_internal.h"

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

// stderr
// ------

ssize_t write_stderr_libnds(int fd, const void *buf, size_t size)
{
    (void)fd;

    if (libnds_stderr_write == NULL)
        return size;

    const char *b = buf;

    for (size_t i = 0; i < size; i++)
        putc_buffered(b[i], stderr_buf, &stderr_buf_len, libnds_stderr_write);

    return size;
}

static char write_buf_err[1]; // BUFSIZ

static struct __file_bufio __stderr =
                FDEV_SETUP_BUFIO(STDERR_FILENO, write_buf_err, sizeof(write_buf_err),
                                 NULL, write_stderr_libnds, NULL, NULL,
                                 __SWR, 0); // Write-only, unbuffered

FILE *const stderr = &__stderr.xfile.cfile.file;

// stdout
// ------

ssize_t write_stdout_libnds(int fd, const void *buf, size_t size)
{
    (void)fd;

    // If stdout is not initialized, the user may have nonetheless initialized
    // a debug console with consoleDebugInit(). Try to fall back to that.
    if (libnds_stdout_write == NULL)
        return write_stderr_libnds(0, buf, size);

    const char *b = buf;

    for (size_t i = 0; i < size; i++)
        putc_buffered(b[i], stdout_buf, &stdout_buf_len, libnds_stdout_write);

    return size;
}

static char write_buf_out[1]; // BUFSIZ

static struct __file_bufio __stdout =
                FDEV_SETUP_BUFIO(STDOUT_FILENO, write_buf_out, sizeof(write_buf_out),
                                 NULL, write_stdout_libnds, NULL, NULL,
                                 __SWR, 0); // Write-only, unbuffered

FILE *const stdout = &__stdout.xfile.cfile.file;

// stdin
// -----

#if 0
// TODO: Replace the stdin definition by something like this:

static char read_buf[1];

static struct __file_bufio __stdin =
                FDEV_SETUP_BUFIO(STDIN_FILENO, read_buf, sizeof(read_buf),
                                 // read, write, lseek, close
                                 libnds_stdin_getc_keyboard, NULL, NULL, NULL,
                                 __SRD, 0);

FILE *const stdin = &__stdin.xfile.cfile.file;
#endif

__attribute__((weak))
int libnds_stdin_getc_keyboard(FILE *file)
{
    (void)file;

    return -1;
}

static FILE __stdin = FDEV_SETUP_STREAM(NULL, libnds_stdin_getc_keyboard, NULL,
                                        _FDEV_SETUP_READ);

FILE *const stdin = &__stdin;
