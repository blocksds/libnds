// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_FILESYSTEM_H__
#define LIBNDS_FILESYSTEM_H__

/// @file filesystem.h
///
/// @brief NitroFS filesystem embedded in a NDS ROM.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/// Initializes NitroFS.
///
/// If basepath or argv[0] is provided, an attempt to read the file system
/// using DLDI will be made first. If that fails, the official cartridge
/// protocol will be used instead.
///
/// @param basepath
///     The .nds file path - NULL to auto-detect.
///
/// @return
///     It returns true on success, false on error.
bool nitroFSInit(const char *basepath);

/// Exits NitroFS.
void nitroFSExit(void);

/// This function initializes a NitroFS lookup cache.
///
/// This lookup cache allows avoiding expensive SD card lookups for large
/// and/or backwards seeks, at the expensive of RAM usage.
///
/// This function will return 0 on non-DLDI/SD NitroFS accesses, as lookup
/// caches are unnecessary in these situations.
///
/// @param max_buffer_size
///     The maximum buffer size, in bytes.
///
/// @return
///     0 if the initialization was successful, a non-zero value on error.
int nitroFSInitLookupCache(uint32_t max_buffer_size);

/// Open a NitroFS file descriptor directly by its FAT offset ID.
///
/// This FAT offset ID can be sourced from functions like @see stat,
/// @see fstat or @see readdir - it is equivalent to the st_ino/d_ino value.
///
/// In all other functions, this file descriptor behaves identically to one
/// sourced from @see open , and should be closed likewise.
///
/// @param id
///     The FAT offset ID of the file (0x0000..0xEFFF).
///
/// @return
///     A valid file descriptor; -1 on error.
int nitroFSOpenById(uint16_t id);

/// Open a NitroFS file directly by its FAT offset ID.
///
/// This FAT offset ID can be sourced from functions like @see stat,
/// @see fstat or @see readdir - it is equivalent to the st_ino/d_ino value.
///
/// In all other functions, this file behaves identically to one sourced from
/// @see fopen , and should be closed likewise.
///
/// @param id
///     The FAT offset ID of the file (0x0000..0xEFFF).
/// @param mode
///     The file open mode. Only "r" and "rb" are supported.
///
/// @return
///     A valid file pointer; NULL on error.
FILE *nitroFSFopenById(uint16_t id, const char *mode);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FILESYSTEM_H__
