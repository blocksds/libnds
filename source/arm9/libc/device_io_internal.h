// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef DEVICE_IO_INTERNAL_H__
#define DEVICE_IO_INTERNAL_H__

#include <stddef.h>

#include <nds/arm9/device_io.h>

typedef void (*VoidFunction)(void);

/// Returns a function pointer inside the specified device index. This function
/// isn't meant to be used directly. Use the DEVIO_GETFN() macro instead.
VoidFunction deviceIoGetFunctionFromIndex(int index, size_t offset);

// This returns the function pointer with name "name" in the device information
#define DEVIO_GETFN(index, name) \
    ((typeof((device_io_t){0}.name)) \
        deviceIoGetFunctionFromIndex(index, offsetof(device_io_t, name)))

/// Returns the device index that corresponds to the specified file path.
///
/// @param path
///     The path to the desired file. If it doesn't include an explicit drive
///     name, it returns the device index that is currently active.
///
/// @return
///     The index that corresponds to the specified drive. If it doesn't
///     correspond to any valid drive, it returns the index of the device active
///     currently. If no device is active, it returns -1.
int deviceIoGetIndexFromPath(const char *path);

#endif // DEVICE_IO_INTERNAL_H__
