// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

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
#include <stdio.h>

#include <nds/ndstypes.h>

/// This function calls fatInit() with the default cache size (5 pages = 20 KB).
///
/// @return It returns true on success, false on error.
WARN_UNUSED_RESULT
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
/// @param cache_size_pages
///     The desired size in pages. One page is made of 8 sectors (512 bytes
///     each, 4KB in total). Values < 0 leave the cache size decision to the
///     FAT filesystem implementation.
/// @param set_as_default_device
///     Ignored, kept for compatibility with libfat.
///
/// @return
///     It returns true on success, false on error.
WARN_UNUSED_RESULT
bool fatInit(int32_t cache_size_pages, bool set_as_default_device);

/// This function mounts the DSi nand if not already mounted by fatInit.
///
/// @warning
///     fatInit must be called before calling this function.
///
/// @param read_only
///     Whether partition should be mounted as read only.
///
/// @return It returns true on success, false on error.
///
/// @note The partition can be made writable/read only at a later time with
///     nand_WriteProtect
WARN_UNUSED_RESULT
bool nandInit(bool read_only);

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
/// @return
///     Returns a string with the path.
WARN_UNUSED_RESULT
char *fatGetDefaultCwd(void);

/// This function returns the default drive ("sd:/" or "fat:/").
///
/// It is extracted from argv[0] if it has been provided by the loader.
///
/// If the format of the path provided by the loader is incorrect, or if no
/// argv[0] was provided, it will default to the root of the filesystem. In the
/// case of DSi, the default drive is the SD card. In the case of DS, the
/// default drive is DLDI.
///
/// The returned string must not be passed to free().
///
/// @return
///     Returns a string with the path.
const char *fatGetDefaultDrive(void);

/// This function initializes a lookup cache on a given FAT file.
/// For NitroFS, use @see nitrofsInitLookupCache instead.
///
/// This lookup cache allows avoiding expensive SD card lookups for large and/or
/// backwards lookups, at the expensive of RAM usage.
///
/// Note that, if the file is opened for writing, using this function will
/// prevent the file's size from being expanded.
///
/// @param fd
///     The file descriptor to initialize. Use fileno(file) for FILE * inputs.
/// @param max_buffer_size
///     The maximum buffer size, in bytes.
///
/// @return
///     0 if the initialization was successful, a non-zero value on error.
int fatInitLookupCache(int fd, uint32_t max_buffer_size);

static inline int fatInitLookupCacheFile(FILE *file, uint32_t max_buffer_size)
{
    return fatInitLookupCache(fileno(file), max_buffer_size);
}

#define FAT_INIT_LOOKUP_CACHE_NOT_SUPPORTED     -1
#define FAT_INIT_LOOKUP_CACHE_OUT_OF_MEMORY     -2
#define FAT_INIT_LOOKUP_CACHE_ALREADY_ALLOCATED -3

// FAT file attributes
#define ATTR_ARCHIVE    0x20 ///< Archive
#define ATTR_DIRECTORY  0x10 ///< Directory
#define ATTR_VOLUME     0x08 ///< Volume (Unused in FatFs)
#define ATTR_SYSTEM     0x04 ///< System
#define ATTR_HIDDEN     0x02 ///< Hidden
#define ATTR_READONLY   0x01 ///< Read only

#define FAT_VOLUME_LABEL_MAX 33 ///< Maximum length of a volume label string.

/// Get the FAT volume label.
///
/// @param name
///    Volume name, such as "fat:" or "sd:".
/// @param label
///    Buffer to store the volume label. This buffer should be at least
///    FAT_VOLUME_LABEL_MAX+1 bytes in size.
///
/// @return
///    True on success, false on error.
bool fatGetVolumeLabel(const char *name, char *label);

/// Set the FAT volume label.
///
/// @param name
///    Volume name, such as "fat:" or "sd:".
/// @param label
///    Buffer to store the volume label.
///
/// @return
///    True on success, false on error.
bool fatSetVolumeLabel(const char *name, const char *label);

/// Get FAT attributes of a file.
///
/// This function works when used on NitroFS.
///
/// On error, this function sets errno to an error code.
///
/// @param file
///     Path to the file.
///
/// @return
///     A combination of ATTR_* flags with the attributes of the file. On error,
///     it returns -1.
int FAT_getAttr(const char *file);

/// Set FAT attributes of a file.
///
/// This function fails when used on NitroFS (it's read-only).
///
/// On error, this function sets errno to an error code.
///
/// @param file
///     Path to the file.
/// @param attr
///     A combination of ATTR_* flags with the new attributes of the file.
///
/// @return
///     0 on success, -1 on error.
int FAT_setAttr(const char *file, uint8_t attr);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FAT_H__
