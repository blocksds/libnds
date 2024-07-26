// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <errno.h>
#include <sys/_timeval.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#include "../libnds_internal.h"

// This file implements stubs for system calls. For more information about it,
// check the documentation of newlib and picolibc:
//
//     https://sourceware.org/newlib/libc.html#Syscalls
//     https://github.com/picolibc/picolibc/blob/main/doc/os.md

void _exit(int rc);

// Single-process system; assume a PID, GID and UID of 1.
pid_t getpid(void)
{
    return 1;
}

gid_t getgid(void)
{
    return 1;
}

gid_t getegid(void)
{
    return 1;
}

uid_t getuid(void)
{
    return 1;
}

uid_t geteuid(void)
{
    return 1;
}

int setgid(gid_t gid)
{
    (void)gid;
    errno = EINVAL;
    return -1;
}

int setegid(gid_t gid)
{
    (void)gid;
    errno = EINVAL;
    return -1;
}

int setuid(uid_t uid)
{
    (void)uid;
    errno = EINVAL;
    return -1;
}

int seteuid(uid_t uid)
{
    (void)uid;
    errno = EINVAL;
    return -1;
}

int kill(pid_t pid, int sig)
{
    // The only process that exists is this process, and it can be killed.
    if (pid == 1)
        _exit(128 + sig);

    errno = ESRCH;
    return -1;
}

clock_t times(struct tms *buf)
{
    (void)buf;

    // TODO

    return -1;
}

int fork(void)
{
    errno = ENOSYS;
    return -1;
}

int gettimeofday(struct timeval *tp, void *tz)
{
    (void)tz;

    if (tp != NULL)
    {
#ifdef ARM9
        tp->tv_sec = *punixTime;
        tp->tv_usec = 0;
#else
        tp->tv_sec = __transferRegion()->unixTime;
        tp->tv_usec = 0;
#endif
    }

    return 0;
}

int execve(const char *name, char *const *argv, char *const *env)
{
    (void)name;
    (void)argv;
    (void)env;

    errno = ENOMEM;
    return -1;
}

int getentropy(void *buffer, size_t length)
{
    (void)buffer;
    (void)length;

    // TODO

    errno = ENOSYS;
    return -1;
}

unsigned long getauxval(unsigned long type)
{
    (void)type;
    errno = EINVAL;
    return 0;
}

// Dummy symbol to allow linking C++ applications. This is only needed to handle
// dynamic shared objects (.so), but they don't exist on the NDS.
void *__dso_handle;
