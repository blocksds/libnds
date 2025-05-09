// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef SYS_STATVFS_H__
#define SYS_STATVFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct statvfs
{
    unsigned long f_bsize;
    unsigned long f_frsize;
    fsblkcnt_t f_blocks;
    fsblkcnt_t f_bfree;
    fsblkcnt_t f_bavail;
    fsfilcnt_t f_files;
    fsfilcnt_t f_ffree;
    fsfilcnt_t f_favail;
    unsigned long f_fsid;
    unsigned long f_flag;
    unsigned long f_namemax;
};

#define ST_RDONLY 0x1
#define ST_NOSUID 0x2

int statvfs(const char *__restrict__ path, struct statvfs *__restrict__ buf);
int fstatvfs(int fd, struct statvfs *buf);

#ifdef __cplusplus
}
#endif

#endif // SYS_STATVFS_H__
