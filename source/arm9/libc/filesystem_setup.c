// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <nds/arm9/sdmmc.h>
#include <nds/memory.h>
#include <nds/system.h>

#include <fat.h>

#include "filesystem_includes.h"
#include "fatfs/cache.h"

#define DEFAULT_SECTORS_PER_PAGE    8 // Each sector is 512 bytes

// Device: "fat:/"
#define FF_NTR_VOLUMES 1
static FATFS fs_info[FF_NTR_VOLUMES] = { 0 };

// Devices: "sd:/", "nand:/", "nand2:/"
static FATFS TWL_BSS_VAR(fs_info_twl)[FF_VOLUMES - FF_NTR_VOLUMES] = { 0 };

static inline FATFS* get_fs_info(size_t index)
{
    if (index >= FF_NTR_VOLUMES)
        return &fs_info_twl[index - FF_NTR_VOLUMES];
    return &fs_info[index];
}

PARTITION VolToPart[FF_VOLUMES] = {
    {0, 0},    /* "0:" ==> Auto discover partition in physical drive 0 (dldi), "fat:" */
    {1, 0},    /* "1:" ==> Auto discover partition in physical drive 1 (DSi sd), "sd:" */
    {2, 1},    /* "2:" ==> 1st partition in physical drive 2 (DSi nand), "nand:" */
    {2, 2},    /* "3:" ==> 2nd partition in physical drive 2 (DSi nand photo partition), "nand2:" */
};

static const char *fat_drive = "fat:/";
static const char *sd_drive = "sd:/";
static const char *nand_drive = "nand:/";
static const char *nand2_drive = "nand2:/";

static bool fat_initialized = false;
static bool nand_mounted = false;

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

    // Find the first ':' and the last '/'

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
        else if (c == '/')
        {
            last_slash_pos = i;
        }
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

    // Ensure that the ':' is followed by a '/'

    if (path[first_colon_pos + 1] != '/')
        goto cleanup;

    // Remove the file name from the path

    path[last_slash_pos + 1] = '\0';
    return path;

cleanup:
    free(path);
    return NULL;
}

// This function returns a pointer allocated that must be freed with free() by
// the caller.
char *fatGetDefaultCwd(void)
{
    // If argv[0] is provided, try to use it.

    const char *argv0 = NULL;
    if (__system_argv->argvMagic == ARGV_MAGIC && __system_argv->argc >= 1)
    {
        argv0 = __system_argv->argv[0];

        char *dirpath = get_dirname(argv0);

        if (dirpath != NULL)
            return dirpath;
    }

    // argv[0] wasn't provided, or the path is invalid. Use the root of the SD
    // or the DLDI device as fallback.

    if (isDSiMode())
    {
        // Only default to the DLDI device if it's explicitly used in argv[0].
        // Under any other condition, default to the internal SD slot.
        if (argv0)
        {
            if (strncmp(argv0, fat_drive, strlen(fat_drive)) == 0)
                return strdup(fat_drive);
        }

        return strdup(sd_drive);
    }
    else
    {
        return strdup(fat_drive);
    }
}

const char *fatGetDefaultDrive(void)
{
    // If argv[0] is provided, try to use it.

    if (__system_argv->argvMagic == ARGV_MAGIC && __system_argv->argc >= 1)
    {
        const char *argv0 = __system_argv->argv[0];

        // Check if the path starts with "sd:/", "fat:/", or neither.
        if (strncmp(argv0, sd_drive, strlen(sd_drive)) == 0)
            return sd_drive;
        else if (strncmp(argv0, nand_drive, strlen(nand_drive)) == 0)
            return nand_drive;
        else if (strncmp(argv0, fat_drive, strlen(fat_drive)) == 0)
            return fat_drive;
    }

    // argv[0] wasn't provided, or the path is invalid. Use the DSi SD card as
    // default on DSi, and DLDI on DS as fallback.

    if (isDSiMode())
        return sd_drive;
    else
        return fat_drive;
}

bool nandInit(bool read_only)
{
    if (!isDSiMode())
    {
        errno = ENODEV;
        return false;
    }

    nand_WriteProtect(read_only);

    static bool mount_attempted = false;
    if (mount_attempted)
        return nand_mounted;

    mount_attempted = true;
    nand_mounted = true;
    FRESULT result = f_mount(get_fs_info(2), nand_drive, 1);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        nand_mounted = false;
    }
    result = f_mount(get_fs_info(3), nand2_drive, 1);
    if (result != FR_OK)
    {
        errno = fatfs_error_to_posix(result);
        nand_mounted = false;
    }
    return nand_mounted;
}

bool fatInit(int32_t cache_size_pages, bool set_as_default_device)
{
    (void)set_as_default_device;

    static bool has_been_called = false;

    if (has_been_called == true)
        return fat_initialized;

    has_been_called = true;

    const char *default_drive = NULL;

    // Try to get a default working directory from argv[0]
    // ---------------------------------------------------

    char *default_cwd = fatGetDefaultCwd();

    // Initialize read cache, shared by all filesystems
    // ------------------------------------------------

    int32_t cache_size_sectors = cache_size_pages * DEFAULT_SECTORS_PER_PAGE;

    // once the cache is initialized, keep it around for the lifetime of the program
    int ret = cache_init(cache_size_sectors);
    if (ret != 0)
    {
        errno = ENOMEM;
        goto cleanup;
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
        bool require_nand = false;
        default_drive = sd_drive;

        if (default_cwd != NULL)
        {
            if (strncmp(default_cwd, fat_drive, strlen(fat_drive)) == 0)
            {
                // This is the unusual case of the ROM being loaded from the
                // DLDI device instead of the internal SD card.
                require_fat = true;
                require_sd = false;
                require_nand = false;
                default_drive = fat_drive;
            }

            if (strncmp(default_cwd, nand_drive, strlen(nand_drive)) == 0)
            {
                // The rom is being loaded from the DSi NAND.
                require_fat = false;
                require_sd = false;
                require_nand = true;
                default_drive = nand_drive;
            }
        }

        // Try to initialize the internal SD slot
        result = f_mount(get_fs_info(1), sd_drive, 1);
        if ((result != FR_OK) && require_sd)
        {
            errno = fatfs_error_to_posix(result);
            goto cleanup;
        }

        // Try to initialize DLDI
        result = f_mount(get_fs_info(0), fat_drive, 1);
        if ((result != FR_OK) && require_fat)
        {
            errno = fatfs_error_to_posix(result);
            goto cleanup;
        }

        if (require_nand)
        {
            if (!nandInit(true))
            {
                goto cleanup;
            }
        }
    }
    else
    {
        // On DS always require DLDI to initialize correctly.
        result = f_mount(get_fs_info(0), fat_drive, 1);
        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            goto cleanup;
        }

        default_drive = fat_drive;
    }

    // Set the initial drive and path inside the drive
    // -----------------------------------------------

    // Try to switch to the default location of the NDS file
    if (chdir(default_cwd) != 0)
    {
        // If it wasn't possible to set the full path of the directory, at least
        // switch to the right drive.
        result = f_chdrive(default_drive);
        if (result != FR_OK)
        {
            errno = fatfs_error_to_posix(result);
            goto cleanup;
        }
    }

    free(default_cwd);
    fat_initialized = true;
    return true;

cleanup:
    free(default_cwd);
    return false;
}

bool fatInitDefault(void)
{
    return fatInit(-1, true);
}

int fatInitLookupCache(int fd, uint32_t max_buffer_size)
{
    if (!FD_IS_FAT(fd))
        return FAT_INIT_LOOKUP_CACHE_NOT_SUPPORTED;

    FIL *f = FD_FAT_UNPACK(fd);
    if (f->cltbl != NULL)
        return FAT_INIT_LOOKUP_CACHE_ALREADY_ALLOCATED;

    // Allocate initial look-up cache area
    // -----------------------------------

    f->cltbl = malloc(max_buffer_size);
    if (f->cltbl == NULL)
        return FAT_INIT_LOOKUP_CACHE_OUT_OF_MEMORY;
    f->cltbl[0] = max_buffer_size / sizeof(DWORD);

    // Initialize look-up cache area
    // -----------------------------

    FRESULT ret = f_lseek(f, CREATE_LINKMAP);
    if (ret == FR_NOT_ENOUGH_CORE)
        return f->cltbl[0];

    // Reduce allocation to match actual cache area size
    // -------------------------------------------------

    DWORD *new_cltbl = realloc(f->cltbl, f->cltbl[0] * sizeof(DWORD));
    if (new_cltbl != NULL)
        f->cltbl = new_cltbl;

    return 0;
}

