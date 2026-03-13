// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef NITROFS_DEVICE_H__
#define NITROFS_DEVICE_H__

#include <stdint.h>
#include <stdio.h>

typedef struct {
    FILE *file; // if NULL, use direct cartridge I/O
    uint32_t fnt_offset;
    uint32_t fat_offset;
    uint16_t current_dir;
    bool use_slot2;
} nitrofs_t;

typedef struct {
    uint32_t offset;
    uint16_t first_file;
    uint16_t parent;
} nitrofs_fnt_entry_t;

typedef struct {
    // position, offset, endofs are defined relative to the beginning of ROM
    // offset, endofs are read directly from the NitroFS FAT
    uint32_t offset;
    uint32_t endofs;
    uint32_t position;
    uint16_t file_index;
} nitrofs_file_t;

typedef struct
{
    // sector size + maximum directory entry size (1 + 127 + 2) + round up to 4
    uint8_t buffer[0x200 + 132];
    // card offset
    uint32_t offset;
    // buffer position
    uint16_t position;
    // offset of last read sector
    uint16_t sector_offset;
    // file index
    uint16_t file_index;
    // dir opened
    uint16_t dir_opened;
    // parent directory
    uint16_t dir_parent;
    // dotdot offset
    int16_t dotdot_offset;
} nitrofs_dir_state_t;

// Forward declarations

bool nitrofs_use_for_path(const char *path);
int32_t nitrofs_path_resolve(const char *path);
void *nitrofs_opendir(const char *name, DIR *dirp);
int nitrofs_closedir(DIR *dirp);
struct dirent *nitrofs_readdir(DIR *dirp);
void nitrofs_rewinddir(DIR *dirp);
void nitrofs_seekdir(DIR *dirp, long loc);
long nitrofs_telldir(DIR *dirp);
int nitrofs_getcwd(char *buf, size_t size);
int nitrofs_chdir(const char *path);
int nitrofs_chdrive(const char *drive);
int nitrofs_access(const char *path, int amode);
int nitrofs_isatty(int fd);
int nitrofs_open(const char *path, int flags, mode_t mode);
ssize_t nitrofs_read(int fd, void *ptr, size_t len);
off_t nitrofs_lseek(int fd, off_t offset, int whence);
int nitrofs_close(int fd);
int nitrofs_stat(const char *name, struct stat *st);
int nitrofs_fstat(int fd, struct stat *st);
int nitrofs_fat_get_attr(const char *name);
int nitrofs_fat_set_attr(const char *file, uint8_t attr);
bool nitrofs_fat_get_short_name_for(const char *path, char *buf);

bool nitrofs_isdrive(const char *name);

#endif // NITROFS_DEVICE_H__
