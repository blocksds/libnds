// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <time.h>

#include "ff.h"
#include "fatfs_internal.h"
#include "filesystem_internal.h"
#include "nitrofs_internal.h"

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

    // POSIX | FatFs
    // ------+----------------------------------------
    // "r"   | FA_READ
    // "r+"  | FA_READ | FA_WRITE
    // "w"   | FA_CREATE_ALWAYS | FA_WRITE
    // "w+"  | FA_CREATE_ALWAYS | FA_WRITE | FA_READ
    // "wx"  | FA_CREATE_NEW | FA_WRITE
    // "w+x" | FA_CREATE_NEW | FA_WRITE | FA_READ
    // "a"   | FA_OPEN_APPEND | FA_WRITE
    // "a+"  | FA_OPEN_APPEND | FA_WRITE | FA_READ

    // POSIX | open()
    // ------+----------------------------------------
    // "r"   | O_RDONLY
    // "r+"  | O_RDWR
    // "w"   | O_WRONLY | O_CREAT | O_TRUNC
    // "w+"  | O_RDWR   | O_CREAT | O_TRUNC
    // "wx"  | O_WRONLY | O_CREAT | O_TRUNC | O_EXCL
    // "w+x" | O_RDWR   | O_CREAT | O_TRUNC | O_EXCL
    // "a"   | O_WRONLY | O_CREAT | O_APPEND
    // "a+"  | O_RDWR   | O_CREAT | O_APPEND

    // O_BINARY and O_TEXT are ignored.

    BYTE mode = 0;

    int can_write = 0;

    switch (flags & (O_RDONLY | O_WRONLY | O_RDWR))
    {
        case O_RDONLY:
            mode = FA_READ;
            break;
        case O_WRONLY:
            can_write = 1;
            mode = FA_WRITE;
            break;
        case O_RDWR:
            can_write = 1;
            mode = FA_READ | FA_WRITE;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    if (nitrofs_use_for_path(path))
    {
        if (can_write)
        {
            errno = EACCES;
            return -1;
        }
        return nitrofs_open(path);
    }

    if (can_write)
    {
        if (flags & O_CREAT)
        {
            if (flags & O_APPEND)
            {
                mode |= FA_OPEN_APPEND; // a | a+
            }
            else if (flags & O_TRUNC)
            {
                // O_EXCL isn't used by the fopen provided by picolibc.
                if (flags & O_EXCL)
                    mode |= FA_CREATE_NEW; // wx | w+x
                else
                    mode |= FA_CREATE_ALWAYS; // w | w+
            }
            else
            {
                // O_APPEND or O_TRUNC must be set if O_CREAT is set
                errno = EINVAL;
                return -1;
            }
        }
        else
        {
            mode |= FA_OPEN_EXISTING; // r+
        }
    }
    else
    {
        mode |= FA_OPEN_EXISTING; // r
    }

    FIL *fp = calloc(1, sizeof(FIL));
    if (fp == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    FRESULT result = f_open(fp, path, mode);

    if (result == FR_OK)
        return FD_FAT_PACK(fp);

    free(fp);
    errno = fatfs_error_to_posix(result);
    return -1;
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

    FIL *fp = FD_FAT_UNPACK(fd);
    UINT bytes_read = 0;

    FRESULT result = f_read(fp, ptr, len, &bytes_read);

    if (result == FR_OK)
        return bytes_read;

    errno = fatfs_error_to_posix(result);
    return -1;
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

    FIL *fp = FD_FAT_UNPACK(fd);
    UINT bytes_written = 0;

    FRESULT result = f_write(fp, ptr, len, &bytes_written);

    if (result == FR_OK)
        return bytes_written;

    errno = fatfs_error_to_posix(result);
    return -1;
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


    FIL *fp = FD_FAT_UNPACK(fd);

    FRESULT result = f_sync(fp);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
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

    FIL *fp = FD_FAT_UNPACK(fd);

    FRESULT result = f_close(fp);

    if (fp->cltbl != NULL)
        free(fp->cltbl);
    free(fp);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
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

    FIL *fp = FD_FAT_UNPACK(fd);

    if (whence == SEEK_END)
    {
        // The file offset is set to the size of the file plus offset bytes
        whence = SEEK_SET;
        offset += f_size(fp);
    }
    else if (whence == SEEK_CUR)
    {
        // The file offset is set to its current location plus offset bytes
        whence = SEEK_SET;
        offset += f_tell(fp);
    }
    else if (whence == SEEK_SET)
    {
        // The file offset is set to offset bytes.
    }
    else
    {
        errno = EINVAL;
        return -1;
    }

    FRESULT result = f_lseek(fp, offset);

    if (result == FR_OK)
        return offset;

    errno = fatfs_error_to_posix(result);
    return -1;
}

_off64_t lseek64(int fd, _off64_t offset, int whence)
{
    return (_off64_t)lseek(fd, (off_t)offset, whence);
}

int unlink(const char *name)
{
    FRESULT result = f_unlink(name);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
}

int rmdir(const char *name)
{
    FRESULT result = f_rmdir(name);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
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

    FILINFO fno = { 0 };
    FRESULT result = f_stat(path, &fno);

    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    // On FatFS, st_dev is either 0 (DLDI) or 1 (DSi SD),
    // while st_ino is the file's starting cluster in FAT.
    st->st_dev = fno.fpdrv;
    st->st_ino = fno.fclust;

    st->st_size = fno.fsize;

#if FF_MAX_SS != FF_MIN_SS
#error "Set the block size to the right value"
#endif
    st->st_blksize = FF_MAX_SS;
    st->st_blocks = (fno.fsize + FF_MAX_SS - 1) / FF_MAX_SS;

    st->st_mode = (fno.fattrib & AM_DIR) ?
                   S_IFDIR : // Directory
                   S_IFREG;  // Regular file

    time_t time = fatfs_fattime_to_timestamp(fno.fdate, fno.ftime);
    time_t crtime = fatfs_fattime_to_timestamp(fno.crdate, fno.crtime);

    st->st_atim.tv_sec = time; // Time of last access
    st->st_mtim.tv_sec = time; // Time of last modification
    st->st_ctim.tv_sec = crtime; // Time of last file entry change (~= creation)

    return 0;
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

    FIL *fp = FD_FAT_UNPACK(fd);

    // On FatFS, st_dev is either 0 (DLDI) or 1 (DSi SD),
    // while st_ino is the file's starting cluster in FAT.
    st->st_dev = fp->obj.fs->pdrv;
    st->st_ino = fp->obj.sclust;
    st->st_size = fp->obj.objsize;

#if FF_MAX_SS != FF_MIN_SS
#error "Set the block size to the right value"
#endif
    st->st_blksize = FF_MAX_SS;
    st->st_blocks = (fp->obj.objsize + FF_MAX_SS - 1) / FF_MAX_SS;

    // An open file will never be anything but a regular file.
    st->st_mode = S_IFREG;

    // TODO: FatFS does not allow running f_stat() on an open file. While some
    // information can be gathered from the open file structure, the timestamp
    // is not among them, so it is not available via fstat().
    time_t time = 0;
    st->st_atim.tv_sec = time; // Time of last access
    st->st_mtim.tv_sec = time; // Time of last modification
    st->st_ctim.tv_sec = time; // Time of last status change

    return 0;
}

int isatty(int fd)
{
    if ((fd == STDIN_FILENO) || (fd == STDOUT_FILENO) || (fd == STDERR_FILENO))
        return 1;

    // We could check if the file descriptor is valid, but that would force us
    // to check socket descriptors, nitrofs, etc. To make things easier, don't
    // check them. Instead of EBADF we will return ENOTTY always.
    errno = ENOTTY;
    return 0;
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

bool fatGetVolumeLabel(const char *name, char *label)
{
    if (name == NULL || label == NULL)
        return false;

    return f_getlabel(name, label, NULL) == FR_OK;
}

bool fatSetVolumeLabel(const char *name, const char *label)
{
    if (name == NULL || label == NULL)
        return false;

    if (strncmp(name, "nand", sizeof("nand") - 1) == 0)
        return false;

    size_t name_length = strlen(name);
    size_t label_length = strlen(label);

    char *buffer = malloc(name_length + label_length + 2);
    if (buffer == NULL)
        return false;

    // Copy volume name, strip slash if necessary
    strcpy(buffer, name);
    if (buffer[name_length - 1] == '/')
        buffer[name_length - 1] = 0;

    // Append destination volume label
    strcat(buffer, label);

    FRESULT result = f_setlabel(buffer);
    free(buffer);
    return result == FR_OK;
}

int FAT_getAttr(const char *file)
{
    if (file == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (nitrofs_use_for_path(file))
        return nitrofs_fat_get_attr(file);

    FILINFO fno = { 0 };
    FRESULT result = f_stat(file, &fno);

    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    return fno.fattrib;
}

int FAT_setAttr(const char *file, uint8_t attr)
{
    if (file == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (nitrofs_use_for_path(file))
    {
        errno = EROFS; // Read-only filesystem
        return -1;
    }

    // Modify all attributes (except for directory and volume)
    BYTE mask = AM_RDO | AM_ARC | AM_SYS | AM_HID;

    FRESULT result = f_chmod(file, attr, mask);

    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    return 0;
}

bool FAT_getShortNameFor(const char *path, char *buf)
{
    if ((path == NULL) || (buf == NULL) || nitrofs_use_for_path(path))
    {
        return false;
    }

    FILINFO fno = { 0 };
    FRESULT result = f_stat(path, &fno);

    if (result != FR_OK)
    {
        return false;
    }

    strncpy(buf, fno.altname, FF_SFN_BUF + 1);
    return true;
}