// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

/// @file nds/arm9/device_io.h
///
/// @brief Support for custom user-implemented filesystems.
///
/// By default, libnds supports the NitroFS and FAT filesystems (DLDI, DSi SD
/// and DSi NAND). The functions in this module allow the developer to define
/// additional filesystems that can be accessed with the standard C functions
/// (like fopen() or stat()).

#ifndef LIBNDS_NDS_DEVICE_IO_H__
#define LIBNDS_NDS_DEVICE_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM9
#error Device I/O is only for the ARM9
#endif

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

/// Maximum number of devices that can be defined by the user
#define DEVICE_IO_MAX_DEVICES 5

/// This is the maximum length of a device I/O drive name
#define DEVICE_IO_MAX_DRIVE_NAME_LENGTH 20

/// This struct defines the interface with the user-provided device.
///
/// It's possible to leave some pointers as NULL, they will simply make the
/// syscall to fail and set errno to ENODEV. The only mandatory callback is
/// isdrive(), all other callbacks are optional. However, you will need at least
/// open(), read() and close() to define an interface that can do anything
/// useful.
///
/// All callbacks in this struct use 26 bit file descriptors. The top 4 bits are
/// reserved because they identify the filesystem type, and the bottom 2 bits are
/// reserved for future use. open() may only use bits 27:2 (inclusive) to define
/// file descriptors.
typedef struct
{
    /// Receives a drive name and returns if it's a valid name for the device.
    ///
    /// A device can have multiple names. For example, you could support all
    /// names that start with "test" so that "test0", "test1" up to "test9" are
    /// valid names for this device.
    ///
    /// @param name
    ///     Drive name without colon or slash. For example, "fat" or "nitro",
    ///     but not "fat:/" or "nitro:".
    /// @return
    ///     If it's a valid name for this device, true. If not, false.
    bool (*isdrive)(const char *name);

    /// Opens the file at the given path and returns a file descriptor.
    ///
    /// The callback is responsible for generating a unique file descriptor for
    /// that device. For example, it's possible to allocate a struct in main RAM
    /// with malloc(), cast it to an int, and use that as file descriptor.
    ///
    /// @warning
    ///     Bits 31:28 and 1:0 of the descriptor are reserved and must be zero.
    ///
    /// @return
    ///     The file descriptor. On return, it returns -1 and sets errno.
    int (*open)(const char *path, int flags, mode_t mode);

    /// It closes the provided file descriptor.
    ///
    /// It must free any memory allocated by open().
    ///
    /// @param fd
    ///     File descriptor to be closed.
    ///
    /// @return
    ///     On success it returns 0. On failure it returns -1 and sets errno.
    int (*close)(int fd);

    ssize_t (*write)(int  fd, const void *ptr, size_t len);
    ssize_t (*read)(int fd, void *ptr, size_t len);
    off_t (*lseek)(int fd, off_t pos, int whence);

    int (*fstat)(int fd, struct stat *st);
    int (*stat)(const char *file, struct stat *st);
    int (*lstat)(const char *file, struct stat *st);

    int (*access)(const char *path, int amode);

    int (*link)(const char *old, const char *new_);
    int (*unlink)(const char *name);
    int (*rename)(const char *old, const char *new_);

    int (*mkdir)(const char *path, mode_t mode);
    int (*chdir)(const char *name);
    int (*rmdir)(const char *name);
    int (*getcwd)(char *buf, size_t size);
    int (*chdrive)(const char *drive);

    /// Opens a directory.
    ///
    /// This function is free to allocate with malloc() its own custom struct
    /// with directory information. The pointer needs to be the value returned
    /// by this function. The other directory functions will be able to access
    /// this data by using `dirp->dp`.
    ///
    /// @param name
    ///     Path to the directory to be opened.
    /// @param dirp
    ///     Pre-allocated DIR struct.
    ///
    /// @return
    ///     It returns a pointer to a user-defined struct that will be saved to
    ///     `dirp->dp`. On error, it returns NULL and sets errno.
    void *(*opendir)(const char *name, DIR *dirp);

    /// Closes a directory
    ///
    /// This function must call free(dirp->dp) if opendir() has allocated it.
    ///
    /// @param dirp
    ///     DIR structure associated to the opened directory.
    ///
    /// @return
    ///     On success it returns 0. On failure it sets errno and returns -1.
    int (*closedir)(DIR *dirp);

    struct dirent *(*readdir)(DIR *dirp);
    void (*rewinddir)(DIR *dirp);
    void (*seekdir)(DIR *dirp, long loc);
    long (*telldir)(DIR *dirp);

    int (*statvfs)(const char *path, struct statvfs *buf);
    int (*fstatvfs)(int fd, struct statvfs *buf);
    int (*fsync)(int fd);
    int (*fdatasync)(int fd);

    int (*ftruncate)(int fd, off_t length);
    int (*truncate)(const char *path, off_t length);

    int (*chmod)(const char *path, mode_t mode);
    int (*fchmod)(int fd, mode_t mode);
    int (*fchmodat)(int dir_fd, const char *path, mode_t mode, int flags);

    int (*utimes)(const char *filename, const struct timeval times[2]);
    int (*lutimes)(const char *filename, const struct timeval times[2]);
    int (*utime)(const char *filename, const struct utimbuf *times);

    int (*isatty)(int fd);

    int (*chown)(const char *path, uid_t owner, gid_t group);
    int (*fchown)(int fd, uid_t owner, gid_t group);
    int (*fchownat)(int dir_fd, const char *path, uid_t owner, gid_t group, int flags);

    int (*symlink)(const char *target, const char *path);
    ssize_t (*readlink)(const char *path, char *buf, size_t length);
}
device_io_t;

/// Adds a new device.
///
/// @param dev
///     The device to be added.
///
/// @return
///     A device index on success, a negative number on error.
int deviceIoAdd(const device_io_t *dev);

/// Removes the device at the specified device index.
///
/// @param index
///     The index of the device to remove (the one returned by deviceIoAdd()).
///
/// @return
///     On success, 0. On error, a negative number.
int deviceIoRemove(int index);

/// Returns the device index that corresponds to the specified drive name.
///
/// @param drive
///     The drive name without colon or slash (for example, "nitro" or "fat").
///
/// @return
///     The index that corresponds to the specified drive. If it doesn't
///     correspond to any valid drive, it returns the index of the device active
///     currently. If no device is active, it returns -1.
int deviceIoGetIndexFromDrive(const char *drive);

/// Returns the device index that corresponds to the specified file path.
///
/// @param path
///     The path to the desired file. If it doesn't include an explicit drive
///     name, it returns the device index that is currently active.
///
/// @return
///     The index that corresponds to the specified drive. If it doesn't
///     correspond to any valid drive, it returns the index of the device active
///     currently. If no device is active, it returns -1.
int deviceIoGetIndexFromPath(const char *path);

/// Returns the device interface at the specified index.
///
/// @param index
///     The device index.
///
/// @return
///     The device interface struct or NULL if it doesn't exist.
const device_io_t *deviceIoGetFromIndex(int index);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_DEVICE_IO_H__
