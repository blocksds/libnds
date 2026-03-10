// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef DEVICE_IO_INTERNAL_H__
#define DEVICE_IO_INTERNAL_H__

#include <stddef.h>

#include <nds/arm9/device_io.h>

typedef void (*VoidFunction)(void);

VoidFunction deviceIoGetFunctionFromIndex(int index, size_t offset);

// This returns the function pointer with name "name" in the device information
#define DEVIO_GETFN(index, name) \
    ((typeof((device_io_t){0}.name)) \
        deviceIoGetFunctionFromIndex(index, offsetof(device_io_t, name)))

#endif // DEVICE_IO_INTERNAL_H__
