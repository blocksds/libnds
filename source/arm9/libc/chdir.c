// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include <nds/arm9/device_io.h>

#include "device_io_internal.h"
#include "filesystem_includes.h"

#include "fat_device.h"
#include "nitrofs_device.h"

int chdir(const char *path)
{
    char *divide = strstr(path, ":/");

    if (divide == NULL)
    {
        // This path doesn't include a drive name, use the current drive

        int (*fn_chdir)(const char *);

        if (current_drive_index == FD_TYPE_NITRO)
            fn_chdir = nitrofs_chdir;
        else if (current_drive_index == FD_TYPE_FAT)
            fn_chdir = fat_chdir;
        else
            fn_chdir = DEVIO_GETFN(current_drive_index, chdir);

        if (fn_chdir == NULL)
            return -1;

        return fn_chdir(path);
    }
    else
    {
        // This path includes a drive name. Split it.

        char drive[DEVICE_IO_MAX_DRIVE_NAME_LENGTH + 1];

        // Size of the drive name
        size_t size = divide - path;
        if (size >= sizeof(drive))
        {
            errno = ENOMEM;
            return -1;
        }

        // Copy drive name
        memcpy(drive, path, size);
        drive[size] = '\0';

        int index = deviceIoGetIndexFromDrive(drive);

        int (*fn_chdir)(const char *);
        int (*fn_chdrive)(const char *);

        if (index == FD_TYPE_NITRO)
        {
            fn_chdir = nitrofs_chdir;
            fn_chdrive = nitrofs_chdrive;
        }
        else if (index == FD_TYPE_FAT)
        {
            fn_chdir = fat_chdir;
            fn_chdrive = fat_chdrive;
        }
        else
        {
            fn_chdir = DEVIO_GETFN(index, chdir);
            fn_chdrive = DEVIO_GETFN(index, chdrive);
        }

        if (fn_chdir == NULL)
        {
            errno = EINVAL;
            return -1;
        }
        if (fn_chdrive != NULL)
        {
            // If the driver doesn't provide a chdrive() function, it may only
            // support one drive name.
            if (fn_chdrive(drive) != 0)
                return -1;
        }

        // Get directory without its path
        char *dir = strdup(divide + 1);
        if (dir == NULL)
        {
            errno = ENOMEM;
            return -1;
        }

        // Set the new drive as default drive
        current_drive_index = index;

        int result = fn_chdir(dir);

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

        int (*fn)(char *, size_t);

        if (current_drive_index == FD_TYPE_NITRO)
            fn = nitrofs_getcwd;
        else if (current_drive_index == FD_TYPE_FAT)
            fn = fat_getcwd;
        else
            fn = DEVIO_GETFN(current_drive_index, getcwd);

        if (fn == NULL)
            return NULL;

        if (fn(buf, size - 1) != 0)
            return NULL;

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
