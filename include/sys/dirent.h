// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka

#ifndef SYS_DIRENT_H__
#define SYS_DIRENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/// UTF-8 necessitates a maximum of three bytes for any UTF-16 codepoint.
#define MAXNAMLEN (255 * 3)

struct dirent
{
    /// Inode number. Implementation-defined.
    ///
    /// For FAT filesystems, this stores the cluster the file is located on.
    /// For NitroFS filesystems, this stores the ID of the file.
    ino_t d_ino;

    /// Index within the directory.
    off_t d_off;

    /// File/directory name.
    char d_name[MAXNAMLEN + 1];

    /// File/directory type.
    ///
    /// For BlocksDS, this will typically be either DT_REG (file), or DT_DIR
    /// (directory).
    unsigned char d_type;

    /// Size of this directory entry.
    unsigned short d_reclen;
};

typedef struct
{
    // Buffer containing the returned directory entry.
    struct dirent dirent;

    // Pointer to native directory structure.
    void *dp;

    // Index within the directory.
    off_t index;

    // Type of native directory structure pointer.
    uint8_t dptype;
} DIR;

/// Unknown file type.
#define DT_UNKNOWN 0

/// Regular file.
#define DT_REG 1

/// Directory.
#define DT_DIR 2

// The following are not used by BlocksDS, but provided for compatibility.
#define DT_FIFO 3
#define DT_CHR 4
#define DT_BLK 5
#define DT_LNK 6
#define DT_SOCK 7

#ifdef __cplusplus
}
#endif

#endif // SYS_DIRENT_H__
