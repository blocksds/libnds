// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <time.h>
#include <utime.h>

#include "ff.h"
#include "fatfs_internal.h"
#include "filesystem_internal.h"
#include "nitrofs_internal.h"

int utimes(const char *filename, const struct timeval times[2])
{
    FILINFO fno;

    if (nitrofs_use_for_path(filename))
    {
        errno = EROFS;
        return -1;
    }

    struct tm *modtime = localtime(&times[1].tv_sec);
    uint32_t modstamp = fatfs_timestamp_to_fattime(modtime);
    fno.ftime = modstamp;
    fno.fdate = modstamp >> 16;

    FRESULT result = f_utime(filename, &fno);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
}

int lutimes(const char *filename, const struct timeval times[2])
{
    // FAT does not implement symbolic links; forward to utimes().
    return utimes(filename, times);
}

int utime(const char *filename, const struct utimbuf *times)
{
    if (times == NULL)
        return -1;

    // Forward to utimes().
    struct timeval otimes[2];
    otimes[1].tv_sec = times->modtime;
    return utimes(filename, otimes);
}
