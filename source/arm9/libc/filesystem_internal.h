// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka
// Copyright (C) 2025 Antonio Niño Díaz

#ifndef FILESYSTEM_INTERNAL_H__
#define FILESYSTEM_INTERNAL_H__

#include <stdbool.h>

#include "ff.h"

// File descriptor pointer parsing
#define FD_TYPE(x) ((((uint32_t) (x)) >> 28) & 0x0F)
#define FD_DESC(x) ((((uint32_t) (x))) & 0x0FFFFFFF)

// Make sure that FD_TYPE_SOCKET is always 0 so that interfacing with lwIP in
// DSWiFi is easier. This is needed because poll() and select() get list of file
// descriptors as input, and it would be slow to convert all of them from
// lwIP-compatible file descriptors to libnds-compatible file descriptors.
// Important: lwIP must never use 0, 1 or 2 as descriptors, they are reserved
// for stdin, stdout and stderr.
#define FD_TYPE_SOCKET 0x0 // Network sockets
#define FD_TYPE_FAT    0x1 // Files opened in DLDI / SD / NAND
#define FD_TYPE_NITRO  0x2 // Files opened in NitroFS
// Important note: Don't use types over 0x7. Values 0x8 to 0xF would create
// file descriptors that are negative values, which could cause unexpected bugs.

#define FD_IS_FAT(x)    (FD_TYPE(x) == FD_TYPE_FAT)
#define FD_IS_NITRO(x)  (FD_TYPE(x) == FD_TYPE_NITRO)
#define FD_IS_SOCKET(x) (FD_TYPE(x) == FD_TYPE_SOCKET)

// Create a file descriptor from a FIL pointer
static inline int FD_FAT_PACK(FIL *f)
{
    return (FD_TYPE_FAT << 28) | (intptr_t)f;
}

// Recover a FIL pointer from a file descriptor
static inline FIL *FD_FAT_UNPACK(int fd)
{
    return (FIL *)FD_DESC(fd);
}

extern bool current_drive_is_nitrofs;

#endif // FILESYSTEM_INTERNAL_H__
