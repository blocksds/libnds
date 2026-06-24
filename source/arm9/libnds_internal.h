// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#ifndef ARM9_LIBNDS_INTERNAL_H__
#define ARM9_LIBNDS_INTERNAL_H__

#include <stdarg.h>
#include <time.h>

#include <nds/arm9/console.h>
#include <nds/arm9/input.h>

extern ConsoleOutFn libnds_stdout_write, libnds_stderr_write;

ssize_t write_stderr_libnds(int fd, const void *buf, size_t size);
ssize_t write_stdout_libnds(int fd, const void *buf, size_t size);

void setTransferInputData(touchPosition *touch, u16 buttons);

extern time_t *punixTime;

// In syscalls_filesystem.c

extern ssize_t (*stdin_fn_read)(int, void *, size_t);
extern int (*stdin_fn_ioctl)(int, unsigned long, va_list);
extern int (*stdin_fn_fcntl)(int, int, va_list);

void libnds_setup_default_stdin_hooks(void);

#endif // ARM9_LIBNDS_INTERNAL_H__
