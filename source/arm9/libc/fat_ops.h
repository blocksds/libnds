// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef FAT_OPS_H__
#define FAT_OPS_H__

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

#endif // FAT_OPS_H__
