// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz
// Copyright (C) 2023 Adrian "asie" Siekierka

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdio-bufio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nds/arm9/keyboard.h>
#include "arm9/libnds_internal.h"

// Buffers so that we can send to the console full ANSI escape sequences.
#define OUTPUT_BUFFER_SIZE 48
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

// In keyboard.c
int keyboardFifoGetc(void);
size_t keyboardFifoStoredCharacters(void);

static char stdin_read_buf[128];

static bool stdin_blocking = true;

static ssize_t libnds_stdin_read(int fd, void *ptr, size_t len)
{
    (void)fd;

    size_t characters_read = 0;
    char *dst = ptr;

    // First, try to read from the FIFO until it's empty
    while (characters_read < len)
    {
        int c = keyboardFifoGetc();

        // No more characters left in the FIFO
        if (c == -1)
            break;

        *dst++ = c;
        characters_read++;

        // End of user input, but save the '\n' to the output
        if (c == '\n')
            return characters_read;
    }

    if (characters_read == len)
        return characters_read;

    if (stdin_blocking)
    {
        // If there weren't enough characters stored in the FIFO and stdin is in
        // blocking mode, wait for the user to provide more characters.

        bool was_shown = keyboardIsVisible();

        if (!was_shown)
            keyboardShow();

        while (true)
        {
            int c = keyboardFifoGetc();

            if (c > 0)
            {
                *dst++ = c;
                characters_read++;
            }

            if (characters_read == len)
                break;

            // End of user input, but save the '\n' to the output
            if (c == '\n')
                break;

            cothread_yield_irq(IRQ_VBLANK);
        }

        if (!was_shown)
            keyboardHide();
    }

    return characters_read;
}

// Hook used so that the garbage collector can remove the default keyboard
// functions (if the default keyboard isn't used) even if stdin is referenced in
// the code.
static ssize_t picolibc_stdin_fn_read_hook(int fd, void *ptr, size_t len)
{
    if (stdin_fn_read == NULL)
    {
        // No device has been hooked to stdin
        errno = EIO;
        return -1;
    }

    return stdin_fn_read(fd, ptr, len);
}

static struct __file_bufio __stdin =
                FDEV_SETUP_BUFIO(STDIN_FILENO, stdin_read_buf, sizeof(stdin_read_buf),
                                 // read, write, lseek, close
                                 picolibc_stdin_fn_read_hook, NULL, NULL, NULL,
                                 __SRD, 0);

FILE *const stdin = &__stdin.xfile.cfile.file;

static int libnds_stdin_ioctl(int fd, unsigned long cmd, va_list ap)
{
    (void)fd;

    if ((int)cmd == FIONBIO)
    {
        // Enable or disable non-blocking mode

        int *opt = va_arg(ap, int*);
        if (*opt)
            stdin_blocking = false;
        else
            stdin_blocking = true;

        return 0;
    }
    else if ((int)cmd == FIONREAD)
    {
        // Return number of characters in buffer
        int *dest = va_arg(ap, int*);
        *dest = keyboardFifoStoredCharacters();

        return 0;
    }

    errno = EINVAL;
    return -1;
}

static int libnds_stdin_fcntl(int fd, int cmd, va_list ap)
{
    (void)fd;

    if (cmd == F_GETFL)
    {
        int flags = (stdin_blocking ? 0 : O_NONBLOCK);
        return flags;
    }
    else if (cmd == F_SETFL)
    {
        int flags = va_arg(ap, int);
        if (flags & O_NONBLOCK)
            stdin_blocking = false;
        else
            stdin_blocking = true;

        return 0;
    }

    errno = EINVAL;
    return -1;
}

void libnds_setup_default_stdin_hooks(void)
{
    stdin_fn_read = libnds_stdin_read;
    stdin_fn_ioctl = libnds_stdin_ioctl;;
    stdin_fn_fcntl = libnds_stdin_fcntl;
}
