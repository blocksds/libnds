// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

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

#include <nds/ndstypes.h>

/// Initializes NitroFS.
///
/// The caller can optionally provide the path of a NDS file to open.
///
/// - If the path is provided by the caller, and the file can't be opened,
///   nitroFSInit() gives up.
/// - If the path isn't provided by the user, and the user has provided argv[0],
///   nitroFSInit() will try to use argv[0]. If that file can't be opened it
///   will try to open the filesystem in other ways before giving up.
///
/// @param basepath
///     The .nds file path. If NULL nitroFSInit() will use argv[0] instead.
///
/// @return
///     It returns true on success, false on error. On error, errno will contain
///     the reason for the failure.
WARN_UNUSED_RESULT
bool nitroFSInit(const char *basepath);

/// Exits NitroFS.
///
/// @return
///     It returns true on success, false on error. If NitroFS hadn't been
///     initialized before, it will also return true.
bool nitroFSExit(void);

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
WARN_UNUSED_RESULT
FILE *nitroFSFopenById(uint16_t id, const char *mode);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FILESYSTEM_H__
