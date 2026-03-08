// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz
// Copyright (C) 2023 Adrian "asie" Siekierka

#include "filesystem_includes.h"

int fat_open(const char *path, int flags, mode_t mode_)
{
    (void)mode_;

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

ssize_t fat_read(int fd, void *ptr, size_t len)
{
    FIL *fp = FD_FAT_UNPACK(fd);
    UINT bytes_read = 0;

    FRESULT result = f_read(fp, ptr, len, &bytes_read);

    if (result == FR_OK)
        return bytes_read;

    errno = fatfs_error_to_posix(result);
    return -1;
}

ssize_t fat_write(int fd, const void *ptr, size_t len)
{
    FIL *fp = FD_FAT_UNPACK(fd);
    UINT bytes_written = 0;

    FRESULT result = f_write(fp, ptr, len, &bytes_written);

    if (result == FR_OK)
        return bytes_written;

    errno = fatfs_error_to_posix(result);
    return -1;
}

int fat_fsync(int fd)
{
    FIL *fp = FD_FAT_UNPACK(fd);

    FRESULT result = f_sync(fp);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
}

int fat_close(int fd)
{
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

off_t fat_lseek(int fd, off_t offset, int whence)
{
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

int fat_unlink(const char *name)
{
    FRESULT result = f_unlink(name);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
}

int fat_rmdir(const char *name)
{
    FRESULT result = f_rmdir(name);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
}

int fat_stat(const char *path, struct stat *st)
{
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

int fat_fstat(int fd, struct stat *st)
{
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

int fat_isatty(int fd)
{
    (void)fd;

    // We could check if the file descriptor is valid, but that would force us
    // to check socket descriptors, nitrofs, etc. To make things easier, don't
    // check them. Instead of EBADF we will return ENOTTY always.
    errno = ENOTTY;
    return 0;
}

int fat_rename(const char *old, const char *new_)
{
    FRESULT result = f_rename(old, new_);

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

int fat_ftruncate(int fd, off_t length)
{
    FIL *fp = FD_FAT_UNPACK(fd);

    if ((size_t)length == (size_t)f_size(fp))
        return 0; // There is nothing to do

    // Preserve the current pointer
    FSIZE_t prev_offset = f_tell(fp);

    int ftruncate_ret = ftruncate_internal(fd, length);
    int ftruncate_errno = errno;

    // Try to return pointer to its previous position even if the truncate
    // function has failed (but return the previous errno value).
    FSIZE_t new_offset = fat_lseek(fd, prev_offset, SEEK_SET);

    if (ftruncate_ret != 0)
    {
        errno = ftruncate_errno;
        return -1;
    }

    if (new_offset != prev_offset)
        return -1;

    return 0;
}

int fat_truncate(const char *path, off_t length)
{
    int fd = fat_open(path, O_RDWR, 0);
    if (fd == -1)
        return -1;

    if (FD_TYPE(fd) != FD_TYPE_FAT)
    {
        fat_close(fd);
        errno = EPERM;
        return -1;
    }

    FIL *fp = FD_FAT_UNPACK(fd);
    if ((size_t)length != (size_t)f_size(fp))
    {
        int ret = ftruncate_internal(fd, length);
        if (ret != 0)
        {
            fat_close(fd);
            return -1;
        }
    }

    int ret = fat_close(fd);
    if (ret != 0)
        return -1;

    return 0;
}

int fat_mkdir(const char *path, mode_t mode)
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

int fat_access(const char *path, int amode)
{
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

// -----------------------------------------------------------------------------

static void _statvfs_populate(FATFS *fs, DWORD nclst, struct statvfs *buf)
{
    uint8_t status = disk_status(fs->pdrv);

#if FF_MAX_SS != FF_MIN_SS
    buf->f_bsize = fs->csize * fs->ssize;
#else
    buf->f_bsize = fs->csize * FF_MAX_SS;
#endif
    buf->f_frsize = buf->f_bsize;
    buf->f_blocks = fs->n_fatent - 2;
    buf->f_bfree = nclst;
    buf->f_bavail = nclst;
    buf->f_files = 0;
    buf->f_ffree = 0;
    buf->f_favail = 0;
    buf->f_fsid = fs->fs_type;
    buf->f_flag = (FF_FS_READONLY || (status & STA_PROTECT)) ? ST_RDONLY : 0;
    buf->f_namemax = fs->fs_type >= FS_FAT32 ? 255 : 12;
}

int fat_statvfs(const char *restrict path, struct statvfs *restrict buf)
{
    FATFS *fs;
    DWORD nclst = 0;
    FRESULT result;

    if ((result = f_getfree(path, &nclst, &fs)) != FR_OK || fs == NULL)
    {
        errno = EIO;
        return -1;
    }

    _statvfs_populate(fs, nclst, buf);
    return 0;
}

int fat_fstatvfs(int fd, struct statvfs *buf)
{
    FATFS *fs;
    DWORD nclst = 0;
    FRESULT result;

    FIL *fp = FD_FAT_UNPACK(fd);
    fs = fp->obj.fs;

    // This is not a standard use of f_getfree - there's a patch
    // in ff.c which makes this (path == NULL, fs provided) work.
    if (fs == NULL || (result = f_getfree(NULL, &nclst, &fs)) != FR_OK)
    {
        errno = EIO;
        return -1;
    }

    _statvfs_populate(fs, nclst, buf);
    return 0;
}

// -----------------------------------------------------------------------------

#define INDEX_NO_ENTRY          -1
#define INDEX_END_OF_DIRECTORY  -2

void *fat_opendir(const char *name, DIR *dirp)
{
    DIRff *dp = calloc(1, sizeof(DIRff));
    if (dp == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    FRESULT result = f_opendir(dp, name);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        free(dp);
        return NULL;
    }

    dirp->index = INDEX_NO_ENTRY;

    return dp;
}

int fat_closedir(DIR *dirp)
{
    DIRff *dp = dirp->dp;
    FRESULT result = f_closedir(dp);

    free(dirp->dp);

    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    return 0;
}

struct dirent *fat_readdir(DIR *dirp)
{
    if (dirp->index <= INDEX_END_OF_DIRECTORY)
    {
        errno = EINVAL;
        return NULL;
    }

    DIRff *dp = dirp->dp;

    FILINFO fno = { 0 };
    FRESULT result = f_readdir(dp, &fno);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return NULL;
    }

    if (fno.fname[0] == '\0')
    {
        // End of directory reached
        dirp->index = INDEX_END_OF_DIRECTORY;
        return NULL;
    }

    dirp->index++;

    struct dirent *ent = &(dirp->dirent);
    ent->d_off = dirp->index;
    ent->d_ino = fno.fclust;

    strncpy(ent->d_name, fno.fname, sizeof(ent->d_name));
    ent->d_name[sizeof(ent->d_name) - 1] = '\0';

    if (fno.fattrib & AM_DIR)
        ent->d_type = DT_DIR; // Directory
    else
        ent->d_type = DT_REG; // Regular file

    return ent;
}

void fat_rewinddir(DIR *dirp)
{
    DIRff *dp = dirp->dp;
    (void)f_rewinddir(dp); // Ignore returned value
    dirp->index = INDEX_NO_ENTRY;
}

void fat_seekdir(DIR *dirp, long loc)
{
    if (dirp->index <= INDEX_END_OF_DIRECTORY) // If we're at the end
        fat_rewinddir(dirp);
    else if (loc < dirp->index) // If we have already passed this entry
        fat_rewinddir(dirp);

    while (1)
    {
        // Check if the entry has been found
        if (dirp->index == loc)
            break;

        struct dirent *entry = fat_readdir(dirp);
        if (entry == NULL)
        {
            fat_rewinddir(dirp);
            break;
        }

        // Check if we reached the end of the directory without finding it
        if (dirp->index == INDEX_END_OF_DIRECTORY)
        {
            fat_rewinddir(dirp);
            break;
        }
    }
}

long fat_telldir(DIR *dirp)
{
    return dirp->index;
}

// -----------------------------------------------------------------------------

int fat_utimes(const char *filename, const struct timeval times[2])
{
    FILINFO fno;

    struct tm *modtime = localtime(&times[1].tv_sec);
    uint32_t modstamp = fatfs_timestamp_to_fattime(modtime);
    fno.ftime = modstamp;
    fno.fdate = modstamp >> 16;

    FRESULT result = f_utime(filename, &fno);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
}

int fat_utime(const char *filename, const struct utimbuf *times)
{
    // Forward to utimes().
    struct timeval otimes[2];
    otimes[1].tv_sec = times->modtime;
    return utimes(filename, otimes);
}

int fat_chdir(const char *path)
{
    FRESULT result = f_chdir(path);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    return 0;
}

int fat_chdrive(const char *drive)
{
    FRESULT result = f_chdrive(drive);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    return 0;
}

int fat_getcwd(char *buf, size_t size)
{
    FRESULT result = f_getcwd(buf, size - 1);
    if (result != FR_OK)
    {
        errno = result == FR_NOT_ENOUGH_CORE ? ERANGE : fatfs_error_to_posix(result);
        return -1;
    }
    return 0;
}
