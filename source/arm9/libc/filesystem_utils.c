// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz
// Copyright (C) 2023 Adrian "asie" Siekierka

#include "filesystem_includes.h"

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

    if (nitrofs_use_for_path(file))
        return nitrofs_fat_get_attr(file);

    FILINFO fno = { 0 };
    FRESULT result = f_stat(file, &fno);

    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    return fno.fattrib;
}

int FAT_setAttr(const char *file, uint8_t attr)
{
    if (file == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (nitrofs_use_for_path(file))
    {
        errno = EROFS; // Read-only filesystem
        return -1;
    }

    // Modify all attributes (except for directory and volume)
    BYTE mask = AM_RDO | AM_ARC | AM_SYS | AM_HID;

    FRESULT result = f_chmod(file, attr, mask);

    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return -1;
    }

    return 0;
}

bool FAT_getShortNameFor(const char *path, char *buf)
{
    if ((path == NULL) || (buf == NULL) || nitrofs_use_for_path(path))
    {
        return false;
    }

    FILINFO fno = { 0 };
    FRESULT result = f_stat(path, &fno);

    if (result != FR_OK)
    {
        return false;
    }

    strncpy(buf, fno.altname, FF_SFN_BUF + 1);
    return true;
}
