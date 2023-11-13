// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_FAT_H__
#define LIBNDS_FAT_H__

/// @file fat.h
///
/// @brief Simple replacement of libfat.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/// This function calls fatInit() with the default cache size (5 pages = 20 KB).
///
/// @return It returns true on success, false on error.
bool fatInitDefault(void);

/// This function initializes the FAT filesystem with a default cache size.
///
/// It works differently in a regular DS than in a DSi:
///
/// - DS:  It will try to use DLDI to initialize access to the SD card of the
///        flashcard. If it isn't possible it returns false. If it succeedes, it
///        returns true.
///
/// - DSi: It will try to initialize access to the internal SD slot, and the SD
///        of the flashcard. It will only return false if the internal slot of
///        the DSi can't be accessed, and it will return true if it can.
///
/// The initial working directory is "fat:/" on the DS (DLDI), and "sd:/" on
/// DSi. On the DSi it is possible to switch between both filesystems with
/// `chdir()`.
///
/// This function can be called multiple times, only the first one has any
/// effect. Any call after the first one returns the value returned the first
/// time.
///
/// @param cache_size_pages The desired size in pages. One page is made of 8
///                         sectors (512 bytes each, 4KB in total).
///                         Values < 0 leave the cache size decision to the
///                         FAT filesystem implementation.
///
/// @param set_as_default_device Ignored, kept for compatibility with libfat.
/// @return It returns true on success, false on error.
bool fatInit(int32_t cache_size_pages, bool set_as_default_device);

/// This function returns the default current working directory.
///
/// It is extracted from argv[0] if it has been provided by the loader. If the
/// format of the path provided by the loader is incorrect, or if no argv[0] was
/// provided, it will default to the root of the filesystem.
///
/// The string is allocated with strdup() internally, so the caller of
/// fatGetDefaultCwd() must use free() to free it.
///
/// For example, this function may return "sd:/folder/" or "fat:/".
///
/// @return Returns a string with the path.
char *fatGetDefaultCwd(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FAT_H__
