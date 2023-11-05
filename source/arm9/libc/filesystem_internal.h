// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#ifndef FILESYSTEM_INTERNAL_H__
#define FILESYSTEM_INTERNAL_H__

#include <stdbool.h>

// File descriptor pointer parsing
#define FD_TYPE(x) ((((uint32_t) (x)) >> 28) & 0x0F)
#define FD_DESC(x) ((((uint32_t) (x))) & 0x0FFFFFFF)
#define FD_TYPE_FAT   0x0
#define FD_TYPE_NITRO 0x1
#define FD_IS_FAT(x) (FD_TYPE(x) == FD_TYPE_FAT)
#define FD_IS_NITRO(x) (FD_TYPE(x) == FD_TYPE_NITRO)

extern bool current_drive_is_nitrofs;

#endif // FILESYSTEM_INTERNAL_H__
