// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// "dirent.h" defines DIR, but "ff.h" defines a different non-standard one.
// Functions in this file need to use their standard prototypes, so it is needed
// to somehow rename the DIR of "ff.h". It's better to keep the original header
// unmodified so that updating it is easier, so this is a hack to rename it just
// in this compilation unit.
#define DIR DIRff
#include "ff.h"
#include "fatfs_internal.h"
#undef DIR
#include "filesystem_internal.h"
#include "nitrofs_internal.h"

// Include "dirent.h" after the FatFs inclusion hack.
#include <dirent.h>

#define INDEX_NO_ENTRY          -1
#define INDEX_END_OF_DIRECTORY  -2

static DIR *alloc_dir(size_t len)
{
    void *dp = calloc(1, len);
    if (dp == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    DIR *dirp = calloc(1, sizeof(DIR));
    if (dirp == NULL)
    {
        free(dp);
        errno = ENOMEM;
        return NULL;
    }

    dirp->dp = dp;
    return dirp;
}

static void free_dir(DIR *dirp)
{
    free(dirp->dp);
    free(dirp);
}

DIR *opendir(const char *name)
{
    bool is_nitrofs = nitrofs_use_for_path(name);

    DIR *dirp = alloc_dir(is_nitrofs ? sizeof(nitrofs_dir_state_t) : sizeof(DIRff));
    if (dirp == NULL)
    {
        // errno setting is handled by alloc_dir()
        return NULL;
    }
    dirp->index = INDEX_NO_ENTRY;

    if (is_nitrofs)
    {
        dirp->dptype = FD_TYPE_NITRO;
        nitrofs_dir_state_t *dp = dirp->dp;

        if (!nitrofs_opendir(dp, name))
            return dirp;
    }
    else
    {
        dirp->dptype = FD_TYPE_FAT;
        DIRff *dp = dirp->dp;

        FRESULT result = f_opendir(dp, name);
        if (result == FR_OK)
            return dirp;

        errno = fatfs_error_to_posix(result);
    }
    free_dir(dirp);
    return NULL;
}

int closedir(DIR *dirp)
{
    if (dirp == NULL)
    {
        errno = EBADF;
        return -1;
    }

    FRESULT result = FR_OK;
    if (dirp->dptype == FD_TYPE_FAT)
    {
        DIRff *dp = dirp->dp;

        result = f_closedir(dp);
    }

    free_dir(dirp);

    if (result == FR_OK)
        return 0;

    errno = fatfs_error_to_posix(result);
    return -1;
}

struct dirent *readdir(DIR *dirp)
{
    if (dirp == NULL)
    {
        errno = EBADF;
        return NULL;
    }

    if (dirp->index <= INDEX_END_OF_DIRECTORY)
    {
        errno = EINVAL;
        return NULL;
    }

    struct dirent *ent = &(dirp->dirent);
    memset(ent, 0, sizeof(struct dirent));
    ent->d_reclen = sizeof(struct dirent);

    if (dirp->dptype == FD_TYPE_NITRO)
    {
        nitrofs_dir_state_t *dp = dirp->dp;
        if (nitrofs_readdir(dp, ent))
        {
            dirp->index = INDEX_END_OF_DIRECTORY;
            return NULL;
        }
        else
        {
            dirp->index++;
            ent->d_off = dirp->index;
        }
        return ent;
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

void rewinddir(DIR *dirp)
{
    if (dirp == NULL)
        return;

    if (dirp->dptype == FD_TYPE_NITRO)
    {
        nitrofs_dir_state_t *dp = dirp->dp;
        nitrofs_rewinddir(dp);
    }
    else
    {
        DIRff *dp = dirp->dp;
        (void)f_rewinddir(dp); // Ignore returned value
    }
    dirp->index = INDEX_NO_ENTRY;
}

void seekdir(DIR *dirp, long loc)
{
    if (dirp == NULL)
        return;

    if (dirp->index <= INDEX_END_OF_DIRECTORY) // If we're at the end
        rewinddir(dirp);
    else if (loc < dirp->index) // If we have already passed this entry
        rewinddir(dirp);

    while (1)
    {
        // Check if the entry has been found
        if (dirp->index == loc)
            break;

        struct dirent *entry = readdir(dirp);
        if (entry == NULL)
        {
            rewinddir(dirp);
            break;
        }

        // Check if we reached the end of the directory without finding it
        if (dirp->index == INDEX_END_OF_DIRECTORY)
        {
            rewinddir(dirp);
            break;
        }
    }
}

long telldir(DIR *dirp)
{
    if (dirp == NULL)
    {
        errno = EBADF;
        return -1;
    }

    return dirp->index;
}
