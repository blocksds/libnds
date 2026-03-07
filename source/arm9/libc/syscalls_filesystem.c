// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include "filesystem_includes.h"

#include "fat_device.h"
#include "nitrofs_device.h"

ssize_t (*socket_fn_write)(int, const void *, size_t) = NULL;
ssize_t (*socket_fn_read)(int, void *, size_t) = NULL;
int (*socket_fn_close)(int) = NULL;

bool current_drive_is_nitrofs = false;

// This file implements stubs for system calls. For more information about it,
// check the documentation of newlib and picolibc:
//
//     https://sourceware.org/newlib/libc.html#Syscalls
//     https://github.com/picolibc/picolibc/blob/main/doc/os.md

int open(const char *path, int flags, ...)
{
    if (path == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (nitrofs_use_for_path(path))
        return nitrofs_open(path, flags, 0);
    else
        return fat_open(path, flags, 0);
}

ssize_t read(int fd, void *ptr, size_t len)
{
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        // Using fread() means we go through the locks of picolibc. picolibc
        // never calls read() when reading from stdin, so this is safe.
        //if (fd == STDIN_FILENO)
        //    return fread(ptr, len, 1, stdin);
        // TODO: fread(x, x, x, stdin) doesn't currently work.

        // STDOUT_FILENO or STDERR_FILENO
        errno = EINVAL;
        return -1;
    }

    if (FD_IS_NITRO(fd))
        return nitrofs_read(fd, ptr, len);

    if (FD_IS_SOCKET(fd))
    {
        if (socket_fn_read != NULL)
            return socket_fn_read(fd, ptr, len);

        return -1;
    }

    return fat_read(fd, ptr, len);
}

ssize_t write(int fd, const void *ptr, size_t len)
{
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        // Using fwrite() means we go through the locks of picolibc. picolibc
        // never calls write() when writing to stdout/stderr, so this is safe.
        if (fd == STDOUT_FILENO)
            return fwrite(ptr, len, 1, stdout);
        if (fd == STDERR_FILENO)
            return fwrite(ptr, len, 1, stderr);

        // STDIN_FILENO
        errno = EINVAL;
        return -1;
    }

    if (FD_IS_NITRO(fd))
    {
        errno = EINVAL;
        return -1;
    }

    if (FD_IS_SOCKET(fd))
    {
        if (socket_fn_write != NULL)
            return socket_fn_write(fd, ptr, len);

        errno = EINVAL;
        return -1;
    }

    return fat_write(fd, ptr, len);
}

int fsync(int fd)
{
    // For NitroFS, fsync() is a no-op. For other/non-filesystem descriptors,
    // fsync() is not allowed.
    if (FD_IS_NITRO(fd))
        return 0;

    if (((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO)) || !FD_IS_FAT(fd))
    {
        errno = EINVAL;
        return -1;
    }

    return fat_fsync(fd);
}

// FatFs doesn't distinguish between metadata and non-metadata synchronization,
// so the fsync() symbol is aliased.
int fdatasync(int fd) __attribute__((alias("fsync")));

int close(int fd)
{
    // The stdio descriptors can't be opened or closed
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        errno = EINVAL;
        return -1;
    }

    if (FD_IS_NITRO(fd))
        return nitrofs_close(fd);

    if (FD_IS_SOCKET(fd))
    {
        if (socket_fn_close != NULL)
            return socket_fn_close(fd);

        return -1;
    }

    return fat_close(fd);
}

off_t lseek(int fd, off_t offset, int whence)
{
    // This function doesn't work on stdin, stdout or stderr
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        errno = EINVAL;
        return -1;
    }

    if (FD_IS_NITRO(fd))
        return nitrofs_lseek(fd, offset, whence);

    return fat_lseek(fd, offset, whence);
}

_off64_t lseek64(int fd, _off64_t offset, int whence)
{
    return (_off64_t)lseek(fd, (off_t)offset, whence);
}

int unlink(const char *name)
{
    if (nitrofs_use_for_path(name))
    {
        errno = EACCES;
        return -1;
    }

    return fat_unlink(name);
}

int rmdir(const char *name)
{
    if (nitrofs_use_for_path(name))
    {
        errno = EACCES;
        return -1;
    }

    return fat_rmdir(name);
}

int stat(const char *path, struct stat *st)
{
    if ((path == NULL) || (st == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    if (nitrofs_use_for_path(path))
        return nitrofs_stat(path, st);

    return fat_stat(path, st);
}

// FatFS/NitroFS does not distinguish symbolic links.
int lstat(const char *path, struct stat *st) __attribute__((alias("stat")));

int fstat(int fd, struct stat *st)
{
    if (st == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    // stdin, stdout and stderr don't work with fstat()
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
        return -1;

    if (FD_IS_NITRO(fd))
        return nitrofs_fstat(fd, st);

    return fat_fstat(fd, st);
}

int isatty(int fd)
{
    if ((fd == STDIN_FILENO) || (fd == STDOUT_FILENO) || (fd == STDERR_FILENO))
        return 1;

    return fat_isatty(fd);
}

int link(const char *old, const char *new)
{
    (void)old;
    (void)new;

    errno = EMLINK;
    return -1;
}

int rename(const char *old, const char *new)
{
    if (nitrofs_use_for_path(old) || nitrofs_use_for_path(new))
    {
        errno = EACCES;
        return -1;
    }

    return fat_rename(old, new);
}

int ftruncate(int fd, off_t length)
{
    // This function doesn't work on stdin, stdout or stderr
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        errno = EINVAL;
        return -1;
    }

    if (FD_TYPE(fd) != FD_TYPE_FAT)
    {
        errno = EPERM;
        return -1;
    }

    return fat_ftruncate(fd, length);
}

int truncate(const char *path, off_t length)
{
    if (nitrofs_use_for_path(path))
    {
        errno = EACCES;
        return -1;
    }

    return fat_truncate(path, length);
}

int mkdir(const char *path, mode_t mode)
{
    if (nitrofs_use_for_path(path))
    {
        errno = EACCES;
        return -1;
    }

    return fat_mkdir(path, mode);
}

int chmod(const char *path, mode_t mode)
{
    // The only attributes that FAT supports are "Read only", "Archive",
    // "System" and "Hidden". This doesn't match very well with UNIX
    // permissions, so this function simply does nothing.

    (void)path;
    (void)mode;

    errno = ENOSYS;
    return -1;
}

int fchmod(int fd, mode_t mode)
{
    (void)fd;
    (void)mode;

    errno = ENOSYS;
    return -1;
}

int fchmodat(int dir_fd, const char *path, mode_t mode, int flags)
{
    (void)dir_fd;
    (void)path;
    (void)mode;
    (void)flags;

    errno = ENOSYS;
    return -1;
}

int chown(const char *path, uid_t owner, gid_t group)
{
    // FAT doesn't support file and group owners.

    (void)path;
    (void)owner;
    (void)group;

    errno = ENOSYS;
    return -1;
}

int fchown(int fd, uid_t owner, gid_t group)
{
    (void)fd;
    (void)owner;
    (void)group;

    errno = ENOSYS;
    return -1;
}

int fchownat(int dir_fd, const char *path, uid_t owner, gid_t group, int flags)
{
    (void)dir_fd;
    (void)path;
    (void)owner;
    (void)group;
    (void)flags;

    errno = ENOSYS;
    return -1;
}

int access(const char *path, int amode)
{
    if (path == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (nitrofs_use_for_path(path))
    {
        if ((amode & W_OK) || (nitrofs_path_resolve(path) < 0))
        {
            errno = EACCES;
            return -1;
        }
        return 0;
    }

    return fat_access(path, amode);
}

ssize_t readlink(const char *path, char *buf, size_t length)
{
    // FAT doesn't support symbolic links.

    (void)path;
    (void)buf;
    (void)length;

    errno = ENOSYS;
    return -1;
}

int symlink(const char *target, const char *path)
{
    (void)target;
    (void)path;

    errno = ENOSYS;
    return -1;
}

int statvfs(const char *restrict path, struct statvfs *restrict buf)
{
    if (nitrofs_use_for_path(path))
    {
        errno = ENOSYS;
        return -1;
    }

    return statvfs(path, buf);
}

int fstatvfs(int fd, struct statvfs *buf)
{
    // This isn't handled here
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
        return -1;

    if (FD_IS_NITRO(fd))
    {
        errno = ENOSYS;
        return -1;
    }

    return fstatvfs(fd, buf);
}

int utimes(const char *filename, const struct timeval times[2])
{
    if (nitrofs_use_for_path(filename))
    {
        errno = EROFS;
        return -1;
    }

    return fat_utimes(filename, times);
}

int lutimes(const char *filename, const struct timeval times[2])
{
    // FAT does not implement symbolic links; forward to utimes().
    return utimes(filename, times);
}

int utime(const char *filename, const struct utimbuf *times)
{
    if (times == NULL)
        return -1;

    if (nitrofs_use_for_path(filename))
        return -1;

    return fat_utime(filename, times);
}
