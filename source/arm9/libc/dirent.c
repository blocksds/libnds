// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include "filesystem_includes.h"

#include "fat_device.h"
#include "nitrofs_device.h"

DIR *opendir(const char *name)
{
    if (name == NULL)
    {
        errno = EINVAL;
        return NULL;
    }

    DIR *dirp = calloc(1, sizeof(DIR));
    if (dirp == NULL)
    {
        errno = ENOMEM;
        return NULL;
    }

    void *dp;

    if (nitrofs_use_for_path(name))
    {
        dirp->dptype = FD_TYPE_NITRO;
        dp = nitrofs_opendir(name, dirp);
    }
    else
    {
        dirp->dptype = FD_TYPE_FAT;
        dp = fat_opendir(name, dirp);
    }

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

    int result;
    if (dirp->dptype == FD_TYPE_FAT)
        result = fat_closedir(dirp);
    else
        result = nitrofs_closedir(dirp);

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

    if (dirp->dptype == FD_TYPE_NITRO)
        return nitrofs_readdir(dirp);
    else // if (dirp->dptype == FD_TYPE_FAT)
        return fat_readdir(dirp);
}

void rewinddir(DIR *dirp)
{
    if (dirp == NULL)
        return;

    if (dirp->dptype == FD_TYPE_NITRO)
        nitrofs_rewinddir(dirp);
    else
        fat_rewinddir(dirp);
}

#define INDEX_NO_ENTRY          -1
#define INDEX_END_OF_DIRECTORY  -2

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
