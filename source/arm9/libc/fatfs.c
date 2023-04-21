// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <nds/memory.h>
#include <nds/system.h>

#include "fatfs/cache.h"
#include "fatfs/ff.h"

#define DEFAULT_CACHE_PAGES         5
#define DEFAULT_SECTORS_PER_PAGE    8 // Each sector is 512 bytes

// Devices: "fat:/", "sd:/", "nitro:/"
static FATFS fs_info[FF_VOLUMES] = { 0 };

static bool fat_initialized = false;
static bool nitrofat_initialized = false;

bool nitrofat_reader_is_arm9 = false;

int fatfs_error_to_posix(FRESULT error)
{
    // The following errno codes have been picked so that they make some sort of
    // sense, but also so that they can be told apart.

    const FRESULT codes[] = {
        // Succeeded
        [FR_OK] = 0,
        // A hard error occurred in the low level disk I/O layer
        [FR_DISK_ERR] = EIO,
        // Assertion failed
        [FR_INT_ERR] = EFAULT,
        // The physical drive cannot work
        [FR_NOT_READY] = ECANCELED,
        // Could not find the file
        [FR_NO_FILE] = ENOENT,
        // Could not find the path
        [FR_NO_PATH] = ENOENT,
        // The path name format is invalid
        [FR_INVALID_NAME] = EINVAL,
        // Access denied due to prohibited access or directory full
        [FR_DENIED] = EACCES,
        // Access denied due to prohibited access
        [FR_EXIST] = EEXIST,
        // The file/directory object is invalid
        [FR_INVALID_OBJECT] = EBADF,
        // The physical drive is write protected
        [FR_WRITE_PROTECTED] = EROFS,
        // The logical drive number is invalid
        [FR_INVALID_DRIVE] = EINVAL,
        // The volume has no work area
        [FR_NOT_ENABLED] = ENOMEM,
        // There is no valid FAT volume
        [FR_NO_FILESYSTEM] = ENODEV,
        // The f_mkfs() aborted due to any problem
        [FR_MKFS_ABORTED] = ENXIO,
        // Could not get a grant to access the volume within defined period
        [FR_TIMEOUT] = ETIME,
        // The operation is rejected according to the file sharing policy
        [FR_LOCKED] = EPERM,
        // LFN working buffer could not be allocated
        [FR_NOT_ENOUGH_CORE] = ENOMEM,
        // Number of open files > FF_FS_LOCK
        [FR_TOO_MANY_OPEN_FILES] = ENOSR,
        // Given parameter is invalid
        [FR_INVALID_PARAMETER] = EINVAL,
    };

    // If this ever happens, there has been a serious error in FatFs
    if (error > FR_INVALID_PARAMETER) // error is unsigned
        return ENOMSG;

    return codes[error];
}

// It takes a full path to a NDS ROM and it creates a new string with the path
// to the directory that contains it. It must be freed by the caller of
// get_dirname().
//
//     sd:/test.nds        -> sd:/
//     sd:/folder/test.nds -> sd:/folder/
//
static char *get_dirname(const char *full_path)
{
    char *path = strdup(full_path);

    if (path == NULL)
        return NULL;

    // Check that the path is valid

    int len = strlen(path);

    // Find the first ';' and the last '/'

    int first_colon_pos = -1;
    int last_slash_pos = -1;

    for (int i = 0; i < len; i++)
    {
        char c = path[i];

        if (c == ':')
        {
            // ':' must come before '/'
            if (last_slash_pos != -1)
                goto cleanup;

            if (first_colon_pos == -1)
                first_colon_pos = i;
        }

        if (c == '/')
            last_slash_pos = i;
    }

    // A valid argv[0] must contain a drive name and a path to a NDS file:
    //
    // Valid:
    //
    //     fat:/test.nds
    //     sd:/folder/test.nds
    //
    // Invalid:
    //
    //     test.nds             | No drive name
    //     folder/test.nds      | No drive name
    //     sd:/                 | No file name
    //     fat:/folder/         | No file name
    //     fat/folder:/test.nds | Invalid drive location
    //     fat/fol:der/test.nds | No drive name

    if ((first_colon_pos == -1) || (last_slash_pos == -1))
        goto cleanup;

    // Ensure that the path doesn't end in a '/' and it has a file name

    if (last_slash_pos == (len - 1))
        goto cleanup;

    // Ensure that the ';' is followed by a '/'

    if (path[first_colon_pos + 1] != '/')
        goto cleanup;

    // Remove the file name from the path

    path[last_slash_pos + 1] = '\0';
    return path;

cleanup:
    free(path);
    return NULL;
}

bool fatInit(uint32_t cache_size_pages, bool set_as_default_device)
{
    (void)set_as_default_device;

    static bool has_been_called = false;

    if (has_been_called == true)
        return fat_initialized;

    has_been_called = true;

    const char *fat_drive = "fat:/";
    const char *sd_drive = "sd:/";
    const char *default_drive = NULL;

    // Try to get argv[0] from the loader
    // ----------------------------------

    const char *argv0 = NULL;
    if (__system_argv->argvMagic == ARGV_MAGIC && __system_argv->argc >= 1)
        argv0 = __system_argv->argv[0];

    // Initialize read cache, shared by all filesystems
    // ------------------------------------------------

    uint32_t cache_size_sectors = cache_size_pages * DEFAULT_SECTORS_PER_PAGE;

    int ret = cache_init(cache_size_sectors);
    if (ret != 0)
    {
        errno = ENOMEM;
        return false;
    }

    // Initialize all possible drives
    // ------------------------------

    // Fail if any of the required drives has failed to initialize (the required
    // drive is usually the one that contains the NDS ROM).

    FRESULT result;

    if (isDSiMode())
    {
        // On DSi there is the internal SD card slot, but it is possible to also
        // have a device that uses DLDI. Normally, only the internal SD slot is
        // required, but it is possible for a ROM to be loaded from a DLDI
        // device. In that case, it makes sense to require that drive to be
        // initialized.
        //
        // In short:
        // - If argv specifies that the location of the ROM is "fat:", default
        //   to DLDI. If it specifies "sd:", or nothing, default to SD.
        // - Try to initialize SD slot and DLDI device.
        // - If the default device can't be initialized, fatInit() has failed.

        bool require_fat = false;
        bool require_sd = true;
        default_drive = sd_drive;

        if (argv0 != NULL)
        {
            if (strncmp(argv0, fat_drive, strlen(fat_drive)) == 0)
            {
                // This is the unusual case of the ROM being loaded from the
                // DLDI device instead of the internal SD card.
                require_fat = true;
                require_sd = false;
                default_drive = fat_drive;
            }
        }

        // Try to initialize the internal SD slot
        result = f_mount(&fs_info[1], sd_drive, 1);
        if ((result != FR_OK) && require_sd)
        {
            errno = fatfs_error_to_posix(result);
            return false;
        }

        // Try to initialize DLDI
        result = f_mount(&fs_info[0], fat_drive, 1);
        if ((result != FR_OK) && require_fat)
        {
            errno = fatfs_error_to_posix(result);
            return false;
        }
    }
    else
    {
        // On DS always require DLDI to initialize correctly.
        result = f_mount(&fs_info[0], fat_drive, 1);
        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            return false;
        }

        default_drive = fat_drive;
    }

    // Set the initial drive and path inside the drive
    // -----------------------------------------------

    bool full_path_set = false;
    if (argv0 != NULL)
    {
        // If argv[0] has been provided, try to set the folder of the NDS ROM as the
        // starting folder.

        char *dirpath = get_dirname(argv0);
        if (dirpath)
        {
            // Switch to that drive and path
            if (chdir(dirpath) == 0)
                full_path_set = true;

            free(dirpath);
        }
    }

    if (!full_path_set)
    {
        // If it wasn't possible to set the full path of the directory, at least
        // switch to the right drive.
        result = f_chdrive(default_drive);
        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            return false;
        }
    }

    fat_initialized = true;

    return true;
}

bool fatInitDefault(void)
{
    return fatInit(DEFAULT_CACHE_PAGES, true);
}

bool nitroFSInit(char **basepath)
{
    static bool has_been_called = false;

    if (has_been_called == true)
        return nitrofat_initialized;

    has_been_called = true;

    (void)basepath;

    const char *nitro_drive = "nitro:/";

    sysSetBusOwners(BUS_OWNER_ARM9, BUS_OWNER_ARM9);

    FRESULT result = f_mount(&fs_info[2], nitro_drive, 1);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        return false;
    }

    nitrofat_initialized = true;

    return true;
}

void nitroFATSetReaderCPU(bool use_arm9)
{
    nitrofat_reader_is_arm9 = use_arm9;
}
