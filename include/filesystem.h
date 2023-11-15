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

/// Initializes NitroFS.
///
/// If basepath or argv[0] is provided, an attempt to read the file system
/// using DLDI will be made first. If that fails, the official cartridge
/// protocol will be used instead.
///
/// @param basepath The .nds file path - NULL to auto-detect.
/// @return It returns true on success, false on error.
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
/// @param max_buffer_size The maximum buffer size, in bytes.
///
/// @return 0 if the initialization was successful, a non-zero value on error.
int nitroFSInitLookupCache(uint32_t max_buffer_size);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FILESYSTEM_H__
