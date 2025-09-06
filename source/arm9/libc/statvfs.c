// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/unistd.h>

#include "ff.h"
#include "diskio.h"
#include "fatfs_internal.h"
#include "filesystem_internal.h"
#include "nitrofs_internal.h"

static void _statvfs_populate(FATFS *fs, DWORD nclst, struct statvfs *buf)
{
    uint8_t status = disk_status(fs->pdrv);

#if FF_MAX_SS != FF_MIN_SS
    buf->f_bsize = fs->csize * fs->ssize;
#else
    buf->f_bsize = fs->csize * FF_MAX_SS;
#endif
    buf->f_frsize = buf->f_bsize;
    buf->f_blocks = fs->n_fatent - 2;
    buf->f_bfree = nclst;
    buf->f_bavail = nclst;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_favail = 0;
    buf->f_fsid = fs->fs_type;
    buf->f_flag = (FF_FS_READONLY || (status & STA_PROTECT)) ? ST_RDONLY : 0;
    buf->f_namemax = fs->fs_type >= FS_FAT32 ? 255 : 12;
}

int statvfs(const char *restrict path, struct statvfs *restrict buf)
{
    FATFS *fs;
    DWORD nclst = 0;
    FRESULT result;

    if (nitrofs_use_for_path(path))
    {
        errno = ENOSYS;
        return -1;
    }

    if ((result = f_getfree(path, &nclst, &fs)) != FR_OK || fs == NULL)
    {
        errno = EIO;
        return -1;
    }

    _statvfs_populate(fs, nclst, buf);
    return 0;
}

int fstatvfs(int fd, struct statvfs *buf)
{
    FATFS *fs;
    DWORD nclst = 0;
    FRESULT result;

    // This isn't handled here
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
        return -1;

    if (FD_IS_NITRO(fd))
    {
        errno = ENOSYS;
        return -1;
    }

    FIL *fp = FD_FAT_UNPACK(fd);
    fs = fp->obj.fs;

    // This is not a standard use of f_getfree - there's a patch
    // in ff.c which makes this (path == NULL, fs provided) work.
    if (fs == NULL || (result = f_getfree(NULL, &nclst, &fs)) != FR_OK)
    {
        errno = EIO;
        return -1;
    }

    _statvfs_populate(fs, nclst, buf);
    return 0;
}
