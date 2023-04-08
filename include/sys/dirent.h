// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright 2007
// International Business Machines Corporation,
// Sony Computer Entertainment, Incorporated,
// Toshiba Corporation,
// All rights reserved.
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define MAXNAMLEN   255

#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2
#define DT_DIR      3
#define DT_BLK      4
#define DT_REG      5
#define DT_LNK      6
#define DT_SOCK     7
#define DT_WHT      8

struct dirent {
    ino_t           d_ino;    // Inode number
    off_t           d_off;    // Value that would be returned by telldir()
    unsigned short  d_reclen; // Length of this record
    unsigned char   d_type;   // Type of file; not supported by all filesystems
    char            d_name[MAXNAMLEN + 1]; // Null-terminated filename
};

typedef struct {
    // Private pointer to internal state of the directory.
    void    *dp;
    // Index of the current entry (for telldir() and seekdir()).
    int     index;
    // Allow one readdir for each opendir, and store the data here.
    struct  dirent dirent;
} DIR;

#ifdef __cplusplus
}
#endif

#endif // _SYS_DIRENT_H
