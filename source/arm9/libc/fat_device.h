// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef FAT_DEVICE_H__
#define FAT_DEVICE_H__

#include "filesystem_includes.h"

int fat_open(const char *path, int flags, mode_t mode_);
ssize_t fat_read(int fd, void *ptr, size_t len);
ssize_t fat_write(int fd, const void *ptr, size_t len);
int fat_fsync(int fd);
int fat_close(int fd);
off_t fat_lseek(int fd, off_t offset, int whence);
int fat_unlink(const char *name);
int fat_rmdir(const char *name);
int fat_stat(const char *path, struct stat *st);
int fat_fstat(int fd, struct stat *st);
int fat_isatty(int fd);
int fat_rename(const char *old, const char *new_);
int fat_ftruncate(int fd, off_t length);
int fat_truncate(const char *path, off_t length);
int fat_mkdir(const char *path, mode_t mode);
int fat_access(const char *path, int amode);
int fat_statvfs(const char *restrict path, struct statvfs *restrict buf);
int fat_fstatvfs(int fd, struct statvfs *buf);
void *fat_opendir(const char *name, DIR *dirp);
int fat_closedir(DIR *dirp);
struct dirent *fat_readdir(DIR *dirp);
void fat_rewinddir(DIR *dirp);
void fat_seekdir(DIR *dirp, long loc);
long fat_telldir(DIR *dirp);
int fat_utimes(const char *filename, const struct timeval times[2]);
int fat_utime(const char *filename, const struct utimbuf *times);
int fat_chdir(const char *path);
int fat_chdrive(const char *drive);

#endif // FAT_DEVICE_H__
