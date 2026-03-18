// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka

#ifndef SYS_DIRENT_H__
#define SYS_DIRENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include_next <sys/dirent.h>

#define MAXNAMLEN NAME_MAX

struct __dirstream
{
    // Buffer containing the returned directory entry.
    struct dirent dirent;

    // Pointer to native directory structure.
    void *dp;

    // Index within the directory.
    off_t index;

    // Type of native directory structure pointer.
    uint8_t dptype;
};

#ifdef __cplusplus
}
#endif

#endif // SYS_DIRENT_H__
