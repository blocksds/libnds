// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz
// Copyright (C) 2023 Adrian "asie" Siekierka

#include "filesystem_includes.h"

#include "device_io_internal.h"
#include "fat_device.h"
#include "nitrofs_device.h"

bool fatGetVolumeLabel(const char *name, char *label)
{
    if (name == NULL || label == NULL)
        return false;

    return f_getlabel(name, label, NULL) == FR_OK;
}

bool fatSetVolumeLabel(const char *name, const char *label)
{
    if (name == NULL || label == NULL)
        return false;

    if (strncmp(name, "nand", sizeof("nand") - 1) == 0)
        return false;

    size_t name_length = strlen(name);
    size_t label_length = strlen(label);

    char *buffer = malloc(name_length + label_length + 2);
    if (buffer == NULL)
        return false;

    // Copy volume name, strip slash if necessary
    strcpy(buffer, name);
    if (buffer[name_length - 1] == '/')
        buffer[name_length - 1] = 0;

    // Append destination volume label
    strcat(buffer, label);

    FRESULT result = f_setlabel(buffer);
    free(buffer);
    return result == FR_OK;
}

int FAT_getAttr(const char *file)
{
    if (file == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(file);

    int (*fn)(const char *);

    if (index == FD_TYPE_NITRO)
        fn = nitrofs_fat_get_attr;
    else if (index == FD_TYPE_FAT)
        fn = fat_get_attr;
    else
        fn = DEVIO_GETFN(index, get_attr);

    if (fn == NULL)
        return -1;

    return fn(file);
}

int FAT_setAttr(const char *file, uint8_t attr)
{
    if (file == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    int index = deviceIoGetIndexFromPath(file);

    int (*fn)(const char *, uint8_t);

    if (index == FD_TYPE_NITRO)
        fn = nitrofs_fat_set_attr;
    else if (index == FD_TYPE_FAT)
        fn = fat_set_attr;
    else
        fn = DEVIO_GETFN(index, set_attr);

    if (fn == NULL)
        return -1;

    return fn(file, attr);
}

bool FAT_getShortNameFor(const char *path, char *buf)
{
    if ((path == NULL) || (buf == NULL))
        return false;

    int index = deviceIoGetIndexFromPath(path);

    bool (*fn)(const char *, char *);

    if (index == FD_TYPE_NITRO)
        fn = nitrofs_fat_get_short_name_for;
    else if (index == FD_TYPE_FAT)
        fn = fat_get_short_name_for;
    else
        fn = DEVIO_GETFN(index, get_short_name_for);

    if (fn == NULL)
        return -1;

    return fn(path, buf);
}
