// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Edoardo Lolletti (edo9300)

#ifndef DEVICE_LIST_H
#define DEVICE_LIST_H

#include <stdbool.h>

#include <nds/memory.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEVICELIST_DEVICE_ID_SD   = 0,
    DEVICELIST_DEVICE_ID_NAND = 1
} DEVICELIST_DEVICE_ID;

typedef enum {
    DEVICELIST_DEVICE_TYPE_PYSICAL        = 0,
    DEVICELIST_DEVICE_TYPE_VIRTUAL_FILE   = 1,
    DEVICELIST_DEVICE_TYPE_VIRTUAL_FOLDER = 2
} DEVICELIST_DEVICE_TYPE;

typedef struct PACKED DeviceListEntry {
    char driveLetter;
    struct PACKED {
        u8 deviceId      : 1; // (0=External SD/MMC Slot, 1=Internal eMMC)
        u8 reserved1     : 2;
        u8 deviceType    : 2; // (0=Physical, 1=Virtual/File, 2=Virtual/Folder, 3=Reserved)
        u8 partition     : 1; // (0=1st, 1=2nd)
        u8 reserved2     : 1;
        bool encrypted   : 1; // (set for eMMC physical devices; not for virtual, not for SD)
    };
    u8 permissions; // (bit1=Write, bit2=Read)
    u8 reserved3;
    char deviceName[0x10]; // (eg. "nand" or "dataPub") (zeropadded)
    char path[0x40]; // (eg. "/" or "nand:/shared1") (zeropadded)
} DeviceListEntry;


typedef struct PACKED DeviceList {
    DeviceListEntry devices[11];
    u8 reserved[0x24];
    char appname[0x40];
} DeviceList;

static_assert(sizeof(DeviceList) == 0x400);

#ifdef ARM7
static inline DeviceList *__DSiDeviceList(void)
{
    if (((u32)__DSiHeader->arm7ideviceList) >= 0x02000000)
        return (DeviceList *)__DSiHeader->arm7ideviceList;
    else
        return NULL;
}
#endif

#ifdef __cplusplus
}
#endif

#endif // DEVICE_LIST_H
