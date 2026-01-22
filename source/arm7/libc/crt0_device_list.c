// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Edoardo Lolletti (edo9300)

#include <stddef.h>
#include <string.h>

#include <nds/system.h>
#include <nds/device_list.h>

#define DEVICE_LIST_APPNAME_LEN (sizeof(((DeviceList *)0)->appname))

TWL_CODE static
size_t get_device_name_len_from_app_name(const char (appname)[DEVICE_LIST_APPNAME_LEN])
{
    for (size_t i = 0; i < DEVICE_LIST_APPNAME_LEN; ++i)
    {
        if (appname[i] == ':')
            return i;
    }
    return 0;
}

typedef struct
{
    struct __argv base;

    // The path provided by the device list entry is 0x40 bytes max.
    // To that we add 3 bytes extra of space:
    // - 1 to have space in case the device is a single letter and is mapped to
    //   "sd" (since it would increase the final string by 1).
    // - 1 to hold the null terminator.
    // - 1 to have an extra null after the path string.
    char cmdline[DEVICE_LIST_APPNAME_LEN + 3];
}
device_list_argv;

static_assert((ARGV_ADDRESS + sizeof(device_list_argv)) < 0x03000000,
              "The device list argv struct would fall outside main ram");

TWL_CODE static LIBNDS_NOINLINE
void check_device_list_internal(void)
{
    if (__system_argv->argvMagic == ARGV_MAGIC)
        return;

    DeviceList *deviceList = __DSiDeviceList();
    if (!deviceList)
        return;

    size_t deviceNameLen = get_device_name_len_from_app_name(deviceList->appname);

    if (deviceNameLen == 0)
        return;

    for (size_t i = 0; i < sizeof(deviceList->devices) / sizeof(deviceList->devices[0]); ++i)
    {
        DeviceListEntry* device = &deviceList->devices[i];

        // Currently we assume that the app path is the full path and the device
        // maps directly to either ds or nand depending on the interpretation of
        // the available resources, the apppath could be considered to be
        // allowed to start from a virtual device, that in turn is a subpath of
        // the sd/nand, this possibility is not accounted for as it would
        // require extra logic and the known programs passing the device list
        // (unlaunch and the system launchers) work with this naive
        // implementation
        if (device->deviceName[deviceNameLen] == '\0' &&
            memcmp(device->deviceName, deviceList->appname, deviceNameLen) == 0)
        {
            // Copy to a stack buffer to avoid possible memory overlapping
            char cmdline[DEVICE_LIST_APPNAME_LEN + 3];
            memset(cmdline, 0, sizeof(cmdline));
            if (device->deviceId == DEVICELIST_DEVICE_ID_SD)
            {
                // Transform the root path to sd:/ (can be sdmc:/, nand:/,
                // nand2:/, etc if launched from hiya)
                cmdline[0] = 's';
                cmdline[1] = 'd';
                memcpy(cmdline + 2, deviceList->appname + deviceNameLen,
                       sizeof(deviceList->appname) - deviceNameLen);
            }
            else
            {
                memcpy(cmdline, deviceList->appname, sizeof(deviceList->appname));
            }
            device_list_argv *argv = (device_list_argv *)__system_argv;
            memcpy(argv->cmdline, cmdline, sizeof(cmdline));
            argv->base.argvMagic = ARGV_MAGIC;
            argv->base.commandLine = argv->cmdline;
            argv->base.length = strlen(argv->cmdline) + 1;
            break;
        }
    }
}

void check_device_list(void)
{
    if (!isDSiMode())
        return;

    check_device_list_internal();
}
