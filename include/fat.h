// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_FAT_H__
#define LIBNDS_FAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

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
///
/// @param set_as_default_device Ignored, kept for compatibility with libfat.
/// @return It returns true on success, false on error.
bool fatInit(uint32_t cache_size_pages, bool set_as_default_device);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FAT_H__
