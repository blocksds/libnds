// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#include <string.h>

#include <nds/arm9/device_io.h>

#include "device_io_internal.h"
#include "file_descriptors.h"

#include "fat_device.h"
#include "nitrofs_device.h"

// Indices 0, 1 and 2 are reserved:
// - 0: Networking sockets
// - 1: FAT
// - 2: NitroFS
#define DEVICE_IO_FIRST_USER_INDEX FD_TYPE_USER_MIN

static const device_io_t *devices[DEVICE_IO_MAX_DEVICES];

// This is set to FAT or NitroFS when they are initialized. By default it should
// be -1 and then it gets set to a valid value when filesystem init function
// wants to change it (by calling chdir()).
int8_t current_drive_index = -1;

int deviceIoAdd(const device_io_t *dev)
{
    if (dev == NULL)
        return -1;

    // Make sure that tere is a valid identification function
    if (dev->isdrive == NULL)
        return -2;

    for (int i = 0; i < DEVICE_IO_MAX_DEVICES; i++)
    {
        if (devices[i] != NULL)
            continue;

        devices[i] = dev;

        int index = i + DEVICE_IO_FIRST_USER_INDEX;

        // If no filesystem has been setup so far, set this one as default
        if (current_drive_index == -1)
            current_drive_index = index;

        return index;
    }

    return -3;
}

int deviceIoRemove(int index)
{
    index -= DEVICE_IO_FIRST_USER_INDEX;

    if ((index >= DEVICE_IO_MAX_DEVICES) || (index < 0))
        return -1;

    if (devices[index] == NULL)
        return -2;

    devices[index] = NULL;

    return 0;
}

int deviceIoGetIndexFromDrive(const char *drive)
{
    if (drive == NULL)
        return -1;

    // First, check if the drive is in the default filesystems

    if (nitrofs_isdrive(drive))
        return FD_TYPE_NITRO;

    if (fat_isdrive(drive))
        return FD_TYPE_FAT;

    for (int i = 0; i < DEVICE_IO_MAX_DEVICES; i++)
    {
        if (devices[i] == NULL)
            continue;

        if (devices[i]->isdrive == NULL)
            continue;

        if (devices[i]->isdrive(drive))
            return i + DEVICE_IO_FIRST_USER_INDEX;
    }

    return -1;
}

int deviceIoGetIndexFromPath(const char *path)
{
    if (path == NULL)
        return -1;

    char device_name[DEVICE_IO_MAX_DRIVE_NAME_LENGTH + 1];

    char *end = strstr(path, ":/");

    // If there's no explicit drive in the path, use the current one
    if (end == NULL)
        return current_drive_index;

    size_t device_name_len = end - path;
    if (device_name_len >= sizeof(device_name))
        return -1;

    strncpy(device_name, path, device_name_len);
    device_name[device_name_len] = '\0';

    return deviceIoGetIndexFromDrive(device_name);
}

const device_io_t *deviceIoGetFromIndex(int index)
{
    index -= DEVICE_IO_FIRST_USER_INDEX;

    if ((index >= DEVICE_IO_MAX_DEVICES) || (index < 0))
        return NULL;

    return devices[index];
}

VoidFunction deviceIoGetFunctionFromIndex(int index, size_t offset)
{
    index -= DEVICE_IO_FIRST_USER_INDEX;

    if ((index >= DEVICE_IO_MAX_DEVICES) || (index < 0))
    {
        errno = ENODEV;
        return NULL;
    }

    uintptr_t ptr_location = offset + (uintptr_t)devices[index];
    uintptr_t fn_pointer = *(const uintptr_t *)ptr_location;

    VoidFunction ptr = (VoidFunction)fn_pointer;
    if (ptr == NULL)
    {
        errno = ENOSYS;
        return NULL;
    }

    return ptr;
}
