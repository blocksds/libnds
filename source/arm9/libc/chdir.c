// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fatfs/ff.h"
#include "fatfs_internal.h"
#include "filesystem_internal.h"
#include "nitrofs_internal.h"

int chdir(const char *path)
{
    char *divide = strstr(path, ":/");
    FRESULT result;

    if (divide == NULL)
    {
        // This path doesn't include a drive name

        result = current_drive_is_nitrofs ? nitrofs_chdir(path) : f_chdir(path);
        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            return -1;
        }

        return 0;
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

        if (!strcmp("nitro:", drive)) {
            current_drive_is_nitrofs = true;
        } else {
            current_drive_is_nitrofs = false;
            result = f_chdrive(drive);
            if (result != FR_OK)
            {
                errno = fatfs_error_to_posix(result);
                return -1;
            }
        }

        // Get directory without its path
        char *dir = strdup(divide + 1);
        if (dir == NULL)
        {
            errno = ENOMEM;
            return -1;
        }

        result = current_drive_is_nitrofs ? nitrofs_chdir(dir) : f_chdir(dir);

        free(dir);

        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            return -1;
        }

        return 0;
    }
}

char *getcwd(char *buf, size_t size)
{
    if (buf == NULL)
    {
        // Extension to POSIX.1-2001 standard. If buf is NULL, the buffer is
        // allocated by getcwd() using malloc(). The size is the one provided by
        // the caller. If the size is zero, it is allocated as big as necessary.
        // As an extension to the , glibc's getcwd() allocates the buffer. The
        // caller must free this buffer after using it.

        int optimize_mem = 0;

        if (size == 0)
        {
            size = PATH_MAX + 1;
            optimize_mem = 1;
        }

        buf = calloc(size, 1);
        if (buf == NULL)
        {
            errno = ENOMEM;
            return NULL;
        }

        char *result = getcwd(buf, size);
        if (result == NULL)
        {
            // errno has already been set
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

        if (current_drive_is_nitrofs)
        {
            if (nitrofs_getcwd(buf, size - 1))
                return NULL;
        }
        else
        {
            FRESULT result = f_getcwd(buf, size - 1);
            if (result != FR_OK)
            {
                errno = result == FR_NOT_ENOUGH_CORE ? ERANGE : fatfs_error_to_posix(result);
                return NULL;
            }
        }
        buf[size - 1] = '\0';

        return buf;
    }
}
