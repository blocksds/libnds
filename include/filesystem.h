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
bool nitroFSExit(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FILESYSTEM_H__
