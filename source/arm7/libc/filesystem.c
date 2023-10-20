// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/_timeval.h>
#include <time.h>

#include "common/libnds_internal.h"

// This file implements ARM7-specific stubs for system calls. For more
// information about it, check the documentation of newlib and picolibc:
//
//     https://sourceware.org/newlib/libc.html#Syscalls
//     https://github.com/picolibc/picolibc/blob/main/doc/os.md

// The ARM7 does not implement a file system.

int isatty(int file)
{
    (void)file;

    return 0;
}

off_t lseek(int fd, off_t offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;

    return 0;
}

_off64_t lseek64(int fd, _off64_t offset, int whence)
{
    return (_off64_t)lseek(fd, (off_t)offset, whence);
}

int open(const char *path, int flags, ...)
{
    (void)path;
    (void)flags;

    return -1;
}

ssize_t read(int fd, void *ptr, size_t len)
{
    (void)fd;
    (void)ptr;
    (void)len;

    errno = EINVAL;
    return 0;
}

ssize_t write(int fd, const void *ptr, size_t len)
{
    (void)fd;
    (void)ptr;
    (void)len;

    errno = EINVAL;
    return 0;
}

int close(int fd)
{
    (void)fd;

    errno = EINVAL;
    return -1;
}

int unlink(const char *name)
{
    (void)name;

    errno = ENOENT;
    return -1;
}

int stat(const char *path, struct stat *st)
{
    (void)path;

    st->st_mode = S_IFCHR;
    return 0;
}

int link(const char *old, const char *new)
{
    (void)old;
    (void)new;

    errno = EMLINK;
    return -1;
}
