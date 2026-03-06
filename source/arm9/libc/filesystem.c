// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include "filesystem_includes.h"

#include "fat_ops.h"

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

    FRESULT result = f_rename(old, new);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
}

static int ftruncate_internal(int fd, off_t length)
{
    // This function assumes that the new length is different from the current
    // one, so it doesn't have any shortcuts in case they are the same. The
    // callers must implement them.

    FIL *fp = FD_FAT_UNPACK(fd);

    FSIZE_t fsize = f_size(fp);

    // If the new size is bigger, it's not enough to use f_lseek to set the
    // pointer to the new size, or to use f_expand. Both of them increase the
    // size of the file, but the contents are undefined. According to the
    // documentation of truncate() the new contents need to be zeroed. The only
    // possible way to do this with FatFs is to simply append zeroes to the end
    // of the file.
    //
    // If the new file is smaller, it is enough to call f_lseek to set the
    // pointer to the new size, and then call f_truncate.

    if ((size_t)length > (size_t)fsize)
    {
        // Expand the file to a bigger size

        FRESULT result = f_lseek(fp, fsize);
        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            return -1;
        }

        FSIZE_t size_diff = length - fsize;

        char zeroes[128] = { 0 };

        while (size_diff > 128)
        {
            if (write(fd, zeroes, 128) == -1)
                return -1;

            size_diff -= 128;
        }

        if (size_diff > 0)
        {
            if (write(fd, zeroes, size_diff) == -1)
                return -1;
        }
    }
    else // if (length < fsize)
    {
        // Truncate the file to a smaller size

        FRESULT result = f_lseek(fp, length);
        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            return -1;
        }

        result = f_truncate(fp);
        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            return -1;
        }
    }

    return 0;
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

    FIL *fp = FD_FAT_UNPACK(fd);

    if ((size_t)length == (size_t)f_size(fp))
        return 0; // There is nothing to do

    // Preserve the current pointer
    FSIZE_t prev_offset = f_tell(fp);

    int ftruncate_ret = ftruncate_internal(fd, length);
    int ftruncate_errno = errno;

    // Try to return pointer to its previous position even if the truncate
    // function has failed (but return the previous errno value).
    FSIZE_t new_offset = lseek(fd, prev_offset, SEEK_SET);

    if (ftruncate_ret != 0)
    {
        errno = ftruncate_errno;
        return -1;
    }

    if (new_offset != prev_offset)
        return -1;

    return 0;
}

int truncate(const char *path, off_t length)
{
    int fd = open(path, O_RDWR);
    if (fd == -1)
        return -1;

    if (FD_TYPE(fd) != FD_TYPE_FAT)
    {
        close(fd);
        errno = EPERM;
        return -1;
    }

    FIL *fp = FD_FAT_UNPACK(fd);
    if ((size_t)length != (size_t)f_size(fp))
    {
        int ret = ftruncate_internal(fd, length);
        if (ret != 0)
        {
            close(fd);
            return -1;
        }
    }

    int ret = close(fd);
    if (ret != 0)
        return -1;

    return 0;
}

int mkdir(const char *path, mode_t mode)
{
    (void)mode; // There are no permissions in FAT filesystems

    if (nitrofs_use_for_path(path))
    {
        errno = EACCES;
        return -1;
    }

    FRESULT result = f_mkdir(path);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    return 0;
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

    FILINFO fno = { 0 };
    FRESULT result = f_stat(path, &fno);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    if (amode != F_OK)
    {
        // Ignore R_OK and X_OK. Always test for read access, and test for write
        // access if requested.
        if ((amode & W_OK) && (fno.fattrib & (AM_RDO | AM_DIR)))
        {
            errno = EACCES;
            return -1;
        }
    }

    return 0;
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
