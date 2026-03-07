// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nds/arm9/sdmmc.h>
#include <nds/memory.h>
#include <nds/system.h>

#include <fat.h>

#include "filesystem_includes.h"
#include "fatfs/cache.h"

int fatfs_error_to_posix(FRESULT error)
{
    // The following errno codes have been picked so that they make some sort of
    // sense, but also so that they can be told apart.

    const FRESULT codes[] =
    {
        // Succeeded
        [FR_OK] = 0,
        // A hard error occurred in the low level disk I/O layer
        [FR_DISK_ERR] = EIO,
        // Assertion failed
        [FR_INT_ERR] = EFAULT,
        // The physical drive cannot work
        [FR_NOT_READY] = ECANCELED,
        // Could not find the file
        [FR_NO_FILE] = ENOENT,
        // Could not find the path
        [FR_NO_PATH] = ENOENT,
        // The path name format is invalid
        [FR_INVALID_NAME] = EINVAL,
        // Access denied due to prohibited access or directory full
        [FR_DENIED] = EACCES,
        // Access denied due to prohibited access
        [FR_EXIST] = EEXIST,
        // The file/directory object is invalid
        [FR_INVALID_OBJECT] = EBADF,
        // The physical drive is write protected
        [FR_WRITE_PROTECTED] = EROFS,
        // The logical drive number is invalid
        [FR_INVALID_DRIVE] = EINVAL,
        // The volume has no work area
        [FR_NOT_ENABLED] = ENOMEM,
        // There is no valid FAT volume
        [FR_NO_FILESYSTEM] = ENODEV,
        // The f_mkfs() aborted due to any problem
        [FR_MKFS_ABORTED] = ENXIO,
        // Could not get a grant to access the volume within defined period
        [FR_TIMEOUT] = ETIME,
        // The operation is rejected according to the file sharing policy
        [FR_LOCKED] = EPERM,
        // LFN working buffer could not be allocated
        [FR_NOT_ENOUGH_CORE] = ENOMEM,
        // Number of open files > FF_FS_LOCK
        [FR_TOO_MANY_OPEN_FILES] = ENOSR,
        // Given parameter is invalid
        [FR_INVALID_PARAMETER] = EINVAL,
    };

    // If this ever happens, there has been a serious error in FatFs
    if (error > FR_INVALID_PARAMETER) // error is unsigned
        return ENOMSG;

    return codes[error];
}

time_t fatfs_fattime_to_timestamp(uint16_t ftime, uint16_t fdate)
{
    struct tm timeinfo = { 0 };
    timeinfo.tm_year   = ((fdate >> 9) + 1980) - 1900;
    timeinfo.tm_mon    = ((fdate >> 5) & 15) - 1;
    timeinfo.tm_mday   = fdate & 31;
    timeinfo.tm_hour   = ftime >> 11;
    timeinfo.tm_min    = (ftime >> 5) & 63;
    timeinfo.tm_sec    = (ftime & 31) * 2;

    time_t time = mktime(&timeinfo);

    // If there is any problem determining the modification timestamp, just leave
    // it empty.
    if (time == (time_t)-1)
        time = 0;

    return time;
}

DWORD fatfs_timestamp_to_fattime(struct tm *stm)
{
    return (DWORD)(stm->tm_year - 80) << 25 |
           (DWORD)(stm->tm_mon + 1) << 21 |
           (DWORD)stm->tm_mday << 16 |
           (DWORD)stm->tm_hour << 11 |
           (DWORD)stm->tm_min << 5 |
           (DWORD)stm->tm_sec >> 1;
}
