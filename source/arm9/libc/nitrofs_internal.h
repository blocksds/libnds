// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#ifndef NITROFS_INTERNAL_H__
#define NITROFS_INTERNAL_H__

#include <stdint.h>
#include <stdio.h>

typedef struct {
    FILE *file; // if NULL, use direct cartridge I/O
    uint32_t fnt_offset;
    uint32_t fat_offset;
    uint16_t current_dir;
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
} nitrofs_dir_state_t;

// Forward declarations
struct dirent;
struct stat;

bool nitrofs_use_for_path(const char *path);
int32_t nitrofs_path_resolve(const char *path);
int nitrofs_opendir(nitrofs_dir_state_t *state, const char *name);
int nitrofs_rewinddir(nitrofs_dir_state_t *state);
int nitrofs_readdir(nitrofs_dir_state_t *state, struct dirent *ent);
int nitrofs_getcwd(char *buf, size_t size);
int nitrofs_chdir(const char *path);
int nitrofs_open(const char *path);
ssize_t nitrofs_read(int fd, void *ptr, size_t len);
off_t nitrofs_lseek(int fd, off_t offset, int whence);
int nitrofs_close(int fd);
int nitrofs_stat(const char *name, struct stat *st);
int nitrofs_fstat(int fd, struct stat *st);

#endif // NITROFS_INTERNAL_H__
