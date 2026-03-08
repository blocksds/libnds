// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include "filesystem_includes.h"

#include "fat_device.h"
#include "nitrofs_device.h"

int chdir(const char *path)
{
    char *divide = strstr(path, ":/");

    if (divide == NULL)
    {
        // This path doesn't include a drive name

        if (current_drive_index == FD_TYPE_NITRO)
            return nitrofs_chdir(path);
        else
            return fat_chdir(path);
    }
    else
    {
        // This path includes a drive name. Split it.

        char drive[10]; // The longest name of a drive is "nitro:"

        // Size of the drive name
        size_t size = divide - path + 2;
        if (size > (10 - 1))
        {
            errno = ENOMEM;
            return -1;
        }

        // Copy drive name
        memcpy(drive, path, size);
        drive[size - 1] = '\0';

        if (strcmp("nitro:", drive) == 0)
        {
            current_drive_index = FD_TYPE_NITRO;
            if (nitrofs_chdrive(drive) != 0)
                return -1;
        }
        else
        {
            current_drive_index = FD_TYPE_FAT;
            if (fat_chdrive(drive) != 0)
                return -1;
        }

        // Get directory without its path
        char *dir = strdup(divide + 1);
        if (dir == NULL)
        {
            errno = ENOMEM;
            return -1;
        }

        int result;

        if (current_drive_index == FD_TYPE_NITRO)
            result = nitrofs_chdir(dir);
        else
            result = fat_chdir(dir);

        free(dir);

        return result;
    }
}

char *getcwd(char *buf, size_t size)
{
    if (buf == NULL)
    {
        // Extension to POSIX.1-2001 standard. If buf is NULL, the buffer is
        // allocated by getcwd() using malloc(). The size is the one provided by
        // the caller. If the size is zero, it is allocated as big as necessary.
        // As an extension to the POSIX.1-2001 standard, glibc's getcwd()
        // allocates the buffer. The caller must free this buffer after it's
        // done using it.

        int optimize_mem = 0;

        if (size == 0)
        {
            size = PATH_MAX + 1;
            optimize_mem = 1;
        }

        buf = malloc(size);
        if (buf == NULL)
        {
            errno = ENOMEM;
            return NULL;
        }

        char *result = getcwd(buf, size);
        if (result == NULL)
        {
            // errno has already been set
            free(buf);
            return NULL;
        }

        if (optimize_mem)
        {
            // Allocate new string that uses just the required space and free
            // the temporary one.
            char *ret = strdup(buf);
            free(buf);
            return ret;
        }
        else
        {
            return buf;
        }
    }
    else
    {
        if (size == 0)
        {
            errno = EINVAL;
            return NULL;
        }

        if (current_drive_index == FD_TYPE_NITRO)
        {
            if (nitrofs_getcwd(buf, size - 1))
                return NULL;
        }
        else
        {
            if (fat_getcwd(buf, size - 1))
                return NULL;
        }
        // This shouldn't be needed, the drive functions should fail if the
        // terminator character doesn't fit in the buffer.
        buf[size - 1] = '\0';

        return buf;
    }
}

char *getwd(char *buf)
{
    return getcwd(buf, PATH_MAX);
}

// glibc extension
char *get_current_dir_name(void)
{
    return getcwd(NULL, 0);
}
