// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include <nds/arm9/device_io.h>
#include <nds/arm9/sassert.h>
#include <nds/exceptions.h>

#include "device_io_internal.h"
#include "filesystem_includes.h"

#include "fat_device.h"
#include "nitrofs_device.h"

ssize_t (*socket_fn_write)(int, const void *, size_t) = NULL;
ssize_t (*socket_fn_read)(int, void *, size_t) = NULL;
int (*socket_fn_close)(int) = NULL;

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

    int index = deviceIoGetIndexFromPath(path);

    int (*fn)(const char *, int, mode_t);

    if (index == FD_TYPE_NITRO)
        fn = nitrofs_open;
    else if (index == FD_TYPE_FAT)
        fn = fat_open;
    else
        fn = DEVIO_GETFN(index, open);

    if (fn == NULL)
        return -1;

    int fd = fn(path, flags, 0);
    if (fd == -1)
        return -1;

    // Ensure that all reserved bits are unused
    if ((fd & 0xF0000003) != 0)
        libndsCrash("Bad device file descriptor");

    return fd | (index << 28);
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

    ssize_t (*fn)(int, void *, size_t);

    if (FD_IS_SOCKET(fd))
        fn = socket_fn_read;
    else if (FD_IS_NITRO(fd))
        fn = nitrofs_read;
    else if (FD_IS_FAT(fd))
        fn = fat_read;
    else
        fn = DEVIO_GETFN(FD_TYPE(fd), read);

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd), ptr, len);
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

    ssize_t (*fn)(int, const void *, size_t);

    if (FD_IS_SOCKET(fd))
    {
        fn = socket_fn_write;
    }
    else if (FD_IS_NITRO(fd))
    {
        errno = EINVAL;
        fn = NULL;
    }
    else if (FD_IS_FAT(fd))
    {
        fn = fat_write;
    }
    else
    {
        fn = DEVIO_GETFN(FD_TYPE(fd), write);
    }

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd), ptr, len);
}

int fsync(int fd)
{
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        errno = EINVAL;
        return -1;
    }

    int (*fn)(int);

    if (FD_IS_NITRO(fd))
    {
        return 0; // Nothing to do
    }
    else if (FD_IS_FAT(fd))
    {
        fn = fat_fsync;
    }
    else
    {
        fn = DEVIO_GETFN(FD_TYPE(fd), fsync);
    }

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd));
}

int fdatasync(int fd)
{
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        errno = EINVAL;
        return -1;
    }

    int (*fn)(int);

    if (FD_IS_NITRO(fd))
    {
        return 0; // Nothing to do
    }
    else if (FD_IS_FAT(fd))
    {
        // FatFs doesn't distinguish between metadata and non-metadata
        // synchronization, so the fsync() symbol is aliased.
        fn = fat_fsync;
    }
    else
    {
        fn = DEVIO_GETFN(FD_TYPE(fd), fdatasync);
    }

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd));
}

int close(int fd)
{
    // The stdio descriptors can't be opened or closed
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        errno = EINVAL;
        return -1;
    }

    int (*fn)(int);

    if (FD_IS_SOCKET(fd))
        fn = socket_fn_close;
    else if (FD_IS_NITRO(fd))
        fn = nitrofs_close;
    else if (FD_IS_FAT(fd))
        fn = fat_close;
    else
        fn = DEVIO_GETFN(FD_TYPE(fd), close);

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd));
}

off_t lseek(int fd, off_t offset, int whence)
{
    // This function doesn't work on stdin, stdout or stderr
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        errno = EINVAL;
        return -1;
    }

    off_t (*fn)(int, off_t, int);

    if (FD_IS_NITRO(fd))
        fn = nitrofs_lseek;
    else if (FD_IS_FAT(fd))
        fn = fat_lseek;
    else
        fn = DEVIO_GETFN(FD_TYPE(fd), lseek);

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd), offset, whence);
}

int unlink(const char *name)
{
    if (name == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(name);

    int (*fn)(const char *);

    if (index == FD_TYPE_NITRO)
    {
        errno = EACCES;
        return -1;
    }
    else if (index == FD_TYPE_FAT)
    {
        fn = fat_unlink;
    }
    else
    {
        fn = DEVIO_GETFN(index, unlink);
    }

    if (fn == NULL)
        return -1;

    return fn(name);
}

int rmdir(const char *name)
{
    if (name == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(name);

    int (*fn)(const char *);

    if (index == FD_TYPE_NITRO)
    {
        errno = EACCES;
        return -1;
    }
    else if (index == FD_TYPE_FAT)
    {
        fn = fat_rmdir;
    }
    else
    {
        fn = DEVIO_GETFN(index, rmdir);
    }

    if (fn == NULL)
        return -1;

    return fn(name);
}

int stat(const char *path, struct stat *st)
{
    if ((path == NULL) || (st == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(path);

    int (*fn)(const char *, struct stat *st);

    if (index == FD_TYPE_NITRO)
        fn = nitrofs_stat;
    else if (index == FD_TYPE_FAT)
        fn = fat_stat;
    else
        fn = DEVIO_GETFN(index, stat);

    if (fn == NULL)
        return -1;

    return fn(path, st);
}

int lstat(const char *path, struct stat *st)
{
    if ((path == NULL) || (st == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(path);

    int (*fn)(const char *, struct stat *st);

    if (index == FD_TYPE_NITRO)
        fn = nitrofs_stat; // NitroFS does not distinguish symbolic links.
    else if (index == FD_TYPE_FAT)
        fn = fat_stat; // FatFS does not distinguish symbolic links.
    else
        fn = DEVIO_GETFN(index, lstat);

    if (fn == NULL)
        return -1;

    return fn(path, st);
}

int fstat(int fd, struct stat *st)
{
    // stdin, stdout and stderr don't work with fstat()
    if (((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO)) || (st == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    int (*fn)(int, struct stat*);

    if (FD_IS_NITRO(fd))
        fn = nitrofs_fstat;
    else if (FD_IS_FAT(fd))
        fn = fat_fstat;
    else
        fn = DEVIO_GETFN(FD_TYPE(fd), fstat);

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd), st);
}

int isatty(int fd)
{
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
        return 1;

    int (*fn)(int);

    if (FD_IS_NITRO(fd))
        fn = nitrofs_isatty;
    else if (FD_IS_FAT(fd))
        fn = fat_isatty;
    else
        fn = DEVIO_GETFN(FD_TYPE(fd), isatty);

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd));
}

int link(const char *old, const char *new)
{
    // TODO: Implement
    (void)old;
    (void)new;

    errno = EMLINK;
    return -1;
}

int rename(const char *old, const char *new_)
{
    if ((old == NULL) || (new_ == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    int index1 = deviceIoGetIndexFromPath(old);
    int index2 = deviceIoGetIndexFromPath(new_);
    if (index1 != index2)
    {
        errno = EXDEV;
        return -1;
    }

    int (*fn)(const char *, const char *);

    if (index1 == FD_TYPE_NITRO)
    {
        errno = EACCES;
        return -1;
    }
    else if (index1 == FD_TYPE_FAT)
    {
        fn = fat_rename;
    }
    else
    {
        fn = DEVIO_GETFN(index1, rename);
    }

    if (fn == NULL)
        return -1;

    return fn(old, new_);
}

int ftruncate(int fd, off_t length)
{
    // This function doesn't work on stdin, stdout or stderr
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
    {
        errno = EINVAL;
        return -1;
    }

    int (*fn)(int, off_t);

    if (FD_IS_NITRO(fd))
    {
        errno = EACCES;
        return -1;
    }
    else if (FD_IS_FAT(fd))
    {
        fn = fat_ftruncate;
    }
    else
    {
        fn = DEVIO_GETFN(FD_TYPE(fd), ftruncate);
    }

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd), length);
}

int truncate(const char *path, off_t length)
{
    if (path == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(path);

    int (*fn)(const char *, off_t);

    if (index == FD_TYPE_NITRO)
    {
        errno = EACCES;
        return -1;
    }
    else if (index == FD_TYPE_FAT)
    {
        fn = fat_truncate;
    }
    else
    {
        fn = DEVIO_GETFN(index, truncate);
    }

    if (fn == NULL)
        return -1;

    return fn(path, length);
}

int mkdir(const char *path, mode_t mode)
{
    if (path == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(path);

    int (*fn)(const char *, mode_t);

    if (index == FD_TYPE_NITRO)
    {
        errno = EACCES;
        return -1;
    }
    else if (index == FD_TYPE_FAT)
    {
        fn = fat_mkdir;
    }
    else
    {
        fn = DEVIO_GETFN(index, mkdir);
    }

    if (fn == NULL)
        return -1;

    return fn(path, mode);
}

int chmod(const char *path, mode_t mode)
{
    // TODO: Implement
    (void)path;
    (void)mode;

    errno = ENOSYS;
    return -1;
}

int fchmod(int fd, mode_t mode)
{
    // TODO: Implement
    (void)fd;
    (void)mode;

    errno = ENOSYS;
    return -1;
}

int fchmodat(int dir_fd, const char *path, mode_t mode, int flags)
{
    // TODO: Implement
    (void)dir_fd;
    (void)path;
    (void)mode;
    (void)flags;

    errno = ENOSYS;
    return -1;
}

int chown(const char *path, uid_t owner, gid_t group)
{
    // TODO: Implement
    (void)path;
    (void)owner;
    (void)group;

    errno = ENOSYS;
    return -1;
}

int fchown(int fd, uid_t owner, gid_t group)
{
    // TODO: Implement
    (void)fd;
    (void)owner;
    (void)group;

    errno = ENOSYS;
    return -1;
}

int fchownat(int dir_fd, const char *path, uid_t owner, gid_t group, int flags)
{
    // TODO: Implement
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

    int index = deviceIoGetIndexFromPath(path);

    int (*fn)(const char *, int);

    if (index == FD_TYPE_NITRO)
        fn = nitrofs_access;
    else if (index == FD_TYPE_FAT)
        fn = fat_access;
    else
        fn = DEVIO_GETFN(index, access);

    if (fn == NULL)
        return -1;

    return fn(path, amode);
}

ssize_t readlink(const char *path, char *buf, size_t length)
{
    // TODO: Implement
    (void)path;
    (void)buf;
    (void)length;

    errno = ENOSYS;
    return -1;
}

int symlink(const char *target, const char *path)
{
    // TODO: Implement
    (void)target;
    (void)path;

    errno = ENOSYS;
    return -1;
}

int statvfs(const char *restrict path, struct statvfs *restrict buf)
{
    if ((path == NULL) || (buf == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(path);

    int (*fn)(const char *, struct statvfs *);

    if (index == FD_TYPE_NITRO)
    {
        // Not implemented
        errno = ENOSYS;
        return -1;
    }
    else if (index == FD_TYPE_FAT)
    {
        fn = fat_statvfs;
    }
    else
    {
        fn = DEVIO_GETFN(index, statvfs);
    }

    if (fn == NULL)
        return -1;

    return fn(path, buf);
}

int fstatvfs(int fd, struct statvfs *buf)
{
    // This isn't handled here
    if ((fd >= STDIN_FILENO) && (fd <= STDERR_FILENO))
        return -1;

    int (*fn)(int, struct statvfs*);

    if (FD_IS_NITRO(fd))
    {
        // Not implemented
        // TODO: Implament?
        errno = ENOSYS;
        return -1;
    }
    else if (FD_IS_FAT(fd))
    {
        fn = fat_fstatvfs;
    }
    else
    {
        fn = DEVIO_GETFN(FD_TYPE(fd), fstatvfs);
    }

    if (fn == NULL)
        return -1;

    return fn(FD_DESC(fd), buf);
}

int utimes(const char *filename, const struct timeval times[2])
{
    if (filename == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(filename);

    int (*fn)(const char *, const struct timeval[2]);

    if (index == FD_TYPE_NITRO)
    {
        // Not implemented
        errno = ENOSYS;
        return -1;
    }
    else if (index == FD_TYPE_FAT)
    {
        fn = fat_utimes;
    }
    else
    {
        fn = DEVIO_GETFN(index, utimes);
    }

    if (fn == NULL)
        return -1;

    return fn(filename, times);
}

int lutimes(const char *filename, const struct timeval times[2])
{
    if (filename == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(filename);

    int (*fn)(const char *, const struct timeval[2]);

    if (index == FD_TYPE_NITRO)
    {
        // Not implemented
        errno = ENOSYS;
        return -1;
    }
    else if (index == FD_TYPE_FAT)
    {
        // FAT does not implement symbolic links
        fn = fat_utimes;
    }
    else
    {
        fn = DEVIO_GETFN(index, lutimes);
    }

    if (fn == NULL)
        return -1;

    return fn(filename, times);
}

int utime(const char *filename, const struct utimbuf *times)
{
    if ((filename == NULL) || (times == NULL))
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(filename);

    int (*fn)(const char *, const struct utimbuf *);

    if (index == FD_TYPE_NITRO)
    {
        // Not implemented
        errno = ENOSYS;
        return -1;
    }
    else if (index == FD_TYPE_FAT)
    {
        fn = fat_utime;
    }
    else
    {
        fn = DEVIO_GETFN(index, utime);
    }

    if (fn == NULL)
        return -1;

    return fn(filename, times);
}

// -----------------------------------------------------------------------------

DIR *opendir(const char *name)
{
    if (name == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    int index = deviceIoGetIndexFromPath(name);

    void *(*fn)(const char *, DIR *);

    if (index == FD_TYPE_NITRO)
        fn = nitrofs_opendir;
    else if (index == FD_TYPE_FAT)
        fn = fat_opendir;
    else
        fn = DEVIO_GETFN(index, opendir);

    if (fn == NULL)
        return NULL;

    DIR *dirp = calloc(1, sizeof(DIR));
    if (dirp == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    void *dp = fn(name, dirp);

    dirp->dptype = index;

    if (dp == NULL)
    {
        free(dirp);
        return NULL;
    }

    dirp->dp = dp;

    return dirp;
}

int closedir(DIR *dirp)
{
    if (dirp == NULL)
    {
        errno = EBADF;
        return -1;
    }

    int (*fn)(DIR *);

    if (dirp->dptype == FD_TYPE_NITRO)
        fn = nitrofs_closedir;
    else if (dirp->dptype == FD_TYPE_FAT)
        fn = fat_closedir;
    else
        fn = DEVIO_GETFN(dirp->dptype, closedir);

    if (fn == NULL)
        return -1;

    int result = fn(dirp);

    free(dirp);

    return result;
}

struct dirent *readdir(DIR *dirp)
{
    if (dirp == NULL)
    {
        errno = EBADF;
        return NULL;
    }

    struct dirent *ent = &(dirp->dirent);
    memset(ent, 0, sizeof(struct dirent));
    ent->d_reclen = sizeof(struct dirent);

    struct dirent *(*fn)(DIR *);

    if (dirp->dptype == FD_TYPE_NITRO)
        fn = nitrofs_readdir;
    else if (dirp->dptype == FD_TYPE_FAT)
        fn = fat_readdir;
    else
        fn = DEVIO_GETFN(dirp->dptype, readdir);

    if (fn == NULL)
        return NULL;

    return fn(dirp);
}

void rewinddir(DIR *dirp)
{
    if (dirp == NULL)
        return;

    void (*fn)(DIR *);

    if (dirp->dptype == FD_TYPE_NITRO)
        fn = nitrofs_rewinddir;
    else if (dirp->dptype == FD_TYPE_FAT)
        fn = fat_rewinddir;
    else
        fn = DEVIO_GETFN(dirp->dptype, rewinddir);

    if (fn == NULL)
        return;

    fn(dirp);
}

void seekdir(DIR *dirp, long loc)
{
    if (dirp == NULL)
        return;

    void (*fn)(DIR *, long);

    if (dirp->dptype == FD_TYPE_NITRO)
        fn = nitrofs_seekdir;
    else if (dirp->dptype == FD_TYPE_FAT)
        fn = fat_seekdir;
    else
        fn = DEVIO_GETFN(dirp->dptype, seekdir);

    if (fn == NULL)
        return;

    fn(dirp, loc);
}

long telldir(DIR *dirp)
{
    if (dirp == NULL)
    {
        errno = EBADF;
        return -1;
    }

    long (*fn)(DIR *);

    if (dirp->dptype == FD_TYPE_NITRO)
        fn = nitrofs_telldir;
    else if (dirp->dptype == FD_TYPE_FAT)
        fn = fat_telldir;
    else
        fn = DEVIO_GETFN(dirp->dptype, telldir);

    if (fn == NULL)
        return -1;

    return fn(dirp);
}
