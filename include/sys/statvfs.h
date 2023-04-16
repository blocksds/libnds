// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H
#if defined( __cplusplus )
extern "C" {
#endif

#include <sys/types.h>

struct statvfs {
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

int statvfs(const char * __restrict__ path, struct statvfs * __restrict__ buf);
int fstatvfs(int fd, struct statvfs *buf);

#if defined( __cplusplus )
} // extern "C"
#endif
#endif // define _SYS_STATVFS_H
