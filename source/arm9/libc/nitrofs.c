// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2024 Antonio Niño Díaz
// Copyright (C) 2023 Adrian "asie" Siekierka

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <aeabi.h>
#include <fat.h>
#include <nds/arm9/card.h>
#include <nds/arm9/sassert.h>
#include <nds/arm9/dldi.h>
#include <nds/card.h>
#include <nds/memory.h>
#include <nds/system.h>

#include "fatfs/cache.h"

// "dirent.h" defines DIR, but "ff.h" defines a different non-standard one.
// Functions in this file need to use their standard prototypes, so it is needed
// to somehow rename the DIR of "ff.h". It's better to keep the original header
// unmodified so that updating it is easier, so this is a hack to rename it just
// in this compilation unit.
#define DIR DIRff
#include "ff.h"
#include "fatfs_internal.h"
#undef DIR
#include "filesystem_internal.h"
#include "nitrofs_internal.h"

// Include "dirent.h" after the FatFs inclusion hack.
#include <dirent.h>

static nitrofs_t nitrofs_local;

/// Configuration
#define ENABLE_DOTDOT_EMULATION
#define MAX_NESTED_SUBDIRS 128

/// Helper functions

bool nitrofs_use_for_path(const char *path)
{
    if (strstr(path, ":/"))
        return !memcmp(path, "nitro:/", 7);
    else
        return current_drive_is_nitrofs;
}

// Symbol defined by the linker
extern char __dtcm_start[];
const uintptr_t DTCM_START = (uintptr_t)__dtcm_start;
const uintptr_t DTCM_END   = DTCM_START + (16 * 1024) - 1;

// Read from NitroFS when it is being read from a file
static ssize_t nitrofs_read_internal_file(void *ptr, size_t offset, size_t len)
{
    fseek(nitrofs_local.file, offset, SEEK_SET);
    return fread(ptr, 1, len, nitrofs_local.file);
}

// Read from NitroFS when it is being read from Slot-2
static ssize_t nitrofs_read_internal_slot2(void *ptr, size_t offset, size_t len)
{
    sysSetCartOwner(BUS_OWNER_ARM9);
    memcpy(ptr, (void *)(0x08000000 + offset), len);
    return len;
}

// Read from NitroFS when it is being read with cartridge commands
static ssize_t nitrofs_read_internal_cart(void *ptr, size_t offset, size_t len)
{
    if (dldiGetMode() == DLDI_MODE_ARM7)
    {
        if ((uintptr_t)ptr >= DTCM_START && (uintptr_t)ptr < DTCM_END)
        {
            // The destination is in DTCM

            void *cache = cache_sector_borrow();

#if FF_MAX_SS != FF_MIN_SS
#error "This code expects a fixed sector size"
#endif
            uint8_t *buff = ptr;

            while (len > 0)
            {
                size_t read_size = len > FF_MAX_SS ? FF_MAX_SS : len;

                cardReadArm7(cache, offset, read_size, __NDSHeader->cardControl13);

                __aeabi_memcpy(buff, cache, read_size);

                len -= read_size;
                offset += read_size;
                buff += read_size;
            }

            return len;
        }
        else
        {
            cardReadArm7(ptr, offset, len, __NDSHeader->cardControl13);
            return len;
        }
    }
    else
    {
        sysSetCardOwner(BUS_OWNER_ARM9);
        cardRead(ptr, offset, len, __NDSHeader->cardControl13);
        return len;
    }
}

// This reads from NitroFS using the right access system
static ssize_t nitrofs_read_internal(void *ptr, size_t offset, size_t len)
{
    if (nitrofs_local.file)
        return nitrofs_read_internal_file(ptr, offset, len);

    if (nitrofs_local.use_slot2)
        return nitrofs_read_internal_slot2(ptr, offset, len);

    return nitrofs_read_internal_cart(ptr, offset, len);
}

/// Directory I/O

static bool nitrofs_dir_state_init(nitrofs_dir_state_t *state, uint16_t dir)
{
    nitrofs_fnt_entry_t fnt_entry;

    nitrofs_read_internal(&fnt_entry, nitrofs_local.fnt_offset + ((dir - 0xF000) * 8), sizeof(fnt_entry));
    state->offset = nitrofs_local.fnt_offset + fnt_entry.offset;
    state->sector_offset = 0;
    state->position = 0;
    state->file_index = fnt_entry.first_file;
    state->dir_opened = dir;
    state->dir_parent = fnt_entry.parent;
#ifdef ENABLE_DOTDOT_EMULATION
    // Create dot and dot-dot entries for subdirectories.
    state->dotdot_offset = dir == 0xF000 ? 0 : -2;
#endif

    if (nitrofs_local.file == NULL)
    {
        // Card reads benefit from word-aligning table accesses.
        state->position = state->offset & 3;
        state->offset -= state->position;
    }

    state->buffer[state->position] = 0;
    nitrofs_read_internal(state->buffer, state->offset, 512);
    return state->buffer[state->position] != 0;
}

static bool nitrofs_dir_state_next(nitrofs_dir_state_t *state)
{
    uint32_t length;
    uint8_t type;

    // skip current entry
    type = state->buffer[state->position];
    if (!type)
        return false;
    length = (type & 0x7F) + (type & 0x80 ? 3 : 1);
    state->position += length;
    if (!(type & 0x80))
        state->file_index++;

    // queue next entry
    type = state->buffer[state->position];
    if (type)
    {
        length = (type & 0x7F) + (type & 0x80 ? 3 : 1);
        if ((state->position - state->sector_offset + length) >= 512)
        {
            uint32_t shift = state->position & ~3;
            uint32_t next_sector_offset = (512 + state->sector_offset) - shift;
            memcpy(state->buffer, state->buffer + shift, next_sector_offset);
            state->offset += 512;
            state->sector_offset = next_sector_offset;
            nitrofs_read_internal(state->buffer + next_sector_offset, state->offset, 512);
            state->position &= 3;
        }
    }

    return true;
}

static uint16_t nitrofs_dir_state_index(nitrofs_dir_state_t *state)
{
    uint8_t type = state->buffer[state->position];
    if (type & 0x80)
    {
        uint32_t pos = state->position + (type & 0x7F) + 1;
        // sub-directory entry
        return state->buffer[pos] | (state->buffer[pos + 1] << 8);
    }
    else
    {
        // file entry
        return state->file_index;
    }
}

static uint16_t nitrofs_dir_parent_index(uint16_t dir)
{
    if (dir <= 0xF000)
        return dir;

    nitrofs_fnt_entry_t fnt_entry;
    nitrofs_read_internal(&fnt_entry, nitrofs_local.fnt_offset + ((dir - 0xF000) * 8), sizeof(fnt_entry));
    return fnt_entry.parent;
}

static int32_t nitrofs_dir_step(uint16_t dir, const char *name)
{
    nitrofs_dir_state_t state;

    if (name[0] == 0 || !strcmp(name, ".") || dir < 0xF000)
        return dir;

    if (!strcmp(name, ".."))
        return nitrofs_dir_parent_index(dir);

    if (!nitrofs_dir_state_init(&state, dir))
        return dir;

    size_t name_len = strlen(name);
    do
    {
        uint8_t type = state.buffer[state.position];
        uint8_t len = type & 0x7F;
        if (len == name_len && !memcmp(name, (const char *)(state.buffer + state.position + 1), len))
            return nitrofs_dir_state_index(&state);
    } while (nitrofs_dir_state_next(&state));

    return -1;
}

int nitrofs_opendir(nitrofs_dir_state_t *state, const char *name)
{
    if (!nitrofs_local.fnt_offset)
    {
        errno = ENODEV;
        return -1;
    }

    int32_t res = nitrofs_path_resolve(name);
    if (res < 0)
    {
        errno = ENOENT;
        return -1;
    }
    nitrofs_dir_state_init(state, res);
    return 0;
}

int nitrofs_rewinddir(nitrofs_dir_state_t *state)
{
    nitrofs_dir_state_init(state, state->dir_opened);
    return 0;
}

int nitrofs_readdir(nitrofs_dir_state_t *state, struct dirent *ent)
{
#ifdef ENABLE_DOTDOT_EMULATION
    // Emit dot and dot-dot entries, if requested.
    if (state->dotdot_offset < 0)
    {
        if (state->dotdot_offset == -2)
        {
            ent->d_name[0] = '.';
            ent->d_name[1] = 0;
            ent->d_ino = state->dir_opened;
        }
        else if (state->dotdot_offset == -1)
        {
            ent->d_name[0] = '.';
            ent->d_name[1] = '.';
            ent->d_name[2] = 0;
            ent->d_ino = state->dir_parent;
        }
        ent->d_type = DT_DIR;
        state->dotdot_offset++;
        return 0;
    }
#endif

    uint8_t type = state->buffer[state->position];

    size_t len = type & 0x7F;
    if (len == 0)
        return -1;
    if (len > sizeof(ent->d_name))
        len = sizeof(ent->d_name);
    strncpy(ent->d_name, (const char *)(state->buffer + state->position + 1), len);
    ent->d_name[sizeof(ent->d_name) - 1] = '\0';

    ent->d_type = (type & 0x80) ? DT_DIR : DT_REG;
    ent->d_ino = nitrofs_dir_state_index(state);

    if (!nitrofs_dir_state_next(state))
        return -1;

    return 0;
}

int32_t nitrofs_path_resolve(const char *path)
{
    int32_t entry;
    if (path[0] == '/')
    {
        // start from root directory
        entry = 0xF000;
        path++;
    }
    else if (!memcmp(path, "nitro:/", 7))
    {
        // start from root directory
        entry = 0xF000;
        path += 7;
    }
    else
    {
        // start from current directory
        entry = nitrofs_local.current_dir;
    }

    char *sep = (char *) path;
    while (sep)
    {
        sep = strchr(path, '/');
        if (sep)
            *sep = 0;
        entry = nitrofs_dir_step(entry, path);
        if (sep)
        {
            *sep = '/';
            path = sep + 1;
        }
        if (entry < 0)
            return entry;
    }
    return entry;
}

int nitrofs_getcwd(char *buf, size_t size)
{
    nitrofs_dir_state_t state;
    uint16_t subdirs[MAX_NESTED_SUBDIRS];
    size_t subdir_count = 0;
    size_t bufpos = 0;

    // make a list of directories to traverse
    int32_t res = nitrofs_local.current_dir;
    while (res > 0xF000)
    {
        if (subdir_count >= MAX_NESTED_SUBDIRS)
            return -1;
        subdirs[subdir_count++] = res;
        res = nitrofs_dir_parent_index(res);
    }
    if (res < 0xF000)
    {
        errno = EINVAL;
        return -1;
    }

    // append "nitro:"
    if (bufpos >= (size - 6))
    {
        errno = ERANGE;
        return -1;
    }
    memcpy(buf, "nitro:", 6);
    bufpos += 6;

    // If we are in the root directory add a slash to form "nitro:/"
    if (subdir_count == 0)
    {
        // append "/"
        if (bufpos >= (size - 2))
        {
            errno = ERANGE;
            return -1;
        }
        buf[bufpos++] = '/';
        buf[bufpos] = '\0';

        return 0;
    }

    uint16_t curr_dir = 0xF000;
    while (subdir_count > 0)
    {
        // append "/"
        if (bufpos >= size)
        {
            errno = ERANGE;
            return -1;
        }
        buf[bufpos++] = '/';

        // open parent directory
        if (!nitrofs_dir_state_init(&state, curr_dir))
        {
            errno = EINVAL;
            return -1;
        }

        // find subdirectory name
        uint16_t next_dir = subdirs[subdir_count - 1];
        do
        {
            if (nitrofs_dir_state_index(&state) == next_dir)
            {
                // directory found, append to string and traverse
                uint8_t type = state.buffer[state.position];
                uint8_t len = type & 0x7F;
                if (bufpos >= (size - len))
                {
                    errno = ERANGE;
                    return -1;
                }
                memcpy(buf + bufpos, state.buffer + state.position + 1, len);
                bufpos += len;
                curr_dir = next_dir;
                subdir_count--;
                break;
            }
        } while (nitrofs_dir_state_next(&state));

        if (curr_dir != next_dir)
        {
            // did not find subdirectory
            errno = EINVAL;
            return -1;
        }
    }

    if (bufpos >= (size - 1))
    {
        errno = ERANGE;
        return -1;
    }
    buf[bufpos] = '\0';
    return 0;
}

int nitrofs_chdir(const char *path)
{
    if (!nitrofs_local.fnt_offset)
        return FR_NO_FILESYSTEM;

    int32_t res = nitrofs_path_resolve(path);
    if (res < 0)
        return FR_NO_PATH;
    nitrofs_local.current_dir = res;
    return FR_OK;
}

/// File I/O

ssize_t nitrofs_read(int fd, void *ptr, size_t len)
{
    nitrofs_file_t *f = (nitrofs_file_t *) FD_DESC(fd);
    size_t remaining = f->endofs - f->position;
    if (len > remaining)
        len = remaining;
    if (len == 0)
        return 0;
    ssize_t result = nitrofs_read_internal(ptr, f->position, len);
    if (result <= 0)
        return result;
    f->position += result;
    return result;
}

off_t nitrofs_lseek(int fd, off_t offset, int whence)
{
    nitrofs_file_t *f = (nitrofs_file_t *) FD_DESC(fd);
    size_t new_position;

    if (whence == SEEK_END)
        new_position = f->endofs + offset;
    else if (whence == SEEK_CUR)
        new_position = f->position + offset;
    else if (whence == SEEK_SET)
        new_position = f->offset + offset;
    else
    {
        errno = EINVAL;
        return (off_t)-1;
    }

    if (new_position < f->offset)
        new_position = f->offset;
    else if (new_position > f->endofs)
        new_position = f->endofs;
    f->position = new_position;
    return new_position - f->offset;
}

int nitrofs_close(int fd)
{
    nitrofs_file_t *f = (nitrofs_file_t *) FD_DESC(fd);
    free(f);
    return 0;
}

static int nitrofs_open_by_id(nitrofs_file_t *f, uint16_t id)
{
    if (id >= 0xF000)
    {
        // not a file
        return -1;
    }
    nitrofs_read_internal(f, nitrofs_local.fat_offset + (id * 8), 8);
    f->position = f->offset;
    f->file_index = id;
    return 0;
}

int nitroFSOpenById(uint16_t id)
{
    nitrofs_file_t *f = malloc(sizeof(nitrofs_file_t));
    if (f == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    int32_t res = nitrofs_open_by_id(f, id);
    if (res < 0)
    {
        free(f);
        errno = ENOENT;
        return -1;
    }

    return FD_DESC(f) | (FD_TYPE_NITRO << 28);
}

FILE *nitroFSFopenById(uint16_t id, const char *mode)
{
    int fd = nitroFSOpenById(id);
    if (fd == -1)
        return NULL;
    return fdopen(fd, mode);
}

int nitrofs_open(const char *name)
{
    if (!nitrofs_local.fnt_offset)
    {
        errno = ENODEV;
        return -1;
    }

    int32_t res = nitrofs_path_resolve(name);
    if (res < 0)
    {
        errno = ENOENT;
        return -1;
    }
    return nitroFSOpenById(res);
}

static int nitrofs_stat_file_internal(nitrofs_file_t *f, struct stat *st)
{
    // On NitroFS, st_dev is always 128, while st_ino is the file's unique ID.
    st->st_dev = 128;
    st->st_ino = f->file_index;
    st->st_size = f->endofs - f->offset;
    st->st_blksize = 0x200;
    st->st_blocks = (st->st_size + 0x200 - 1) / 0x200;
    st->st_mode = S_IFREG;

    st->st_atim.tv_sec = 0; // Time of last access
    st->st_mtim.tv_sec = 0; // Time of last modification
    st->st_ctim.tv_sec = 0; // Time of last status change

    return 0;
}

int nitrofs_fat_get_attr(const char *name)
{
    if (!nitrofs_local.fnt_offset)
    {
        errno = ENODEV;
        return -1;
    }

    int32_t res = nitrofs_path_resolve(name);
    if (res < 0)
    {
        errno = ENOENT;
        return -1;
    }

    if (res >= 0xF000)
        return ATTR_DIRECTORY | ATTR_READONLY;

    return ATTR_READONLY;
}

int nitrofs_stat(const char *name, struct stat *st)
{
    if (!nitrofs_local.fnt_offset)
    {
        errno = ENODEV;
        return -1;
    }

    nitrofs_file_t f;
    int32_t res = nitrofs_path_resolve(name);
    if (res < 0)
    {
        errno = ENOENT;
        return -1;
    }
    else if (res >= 0xF000)
    {
        st->st_ino = res;
        st->st_size = 0;
        st->st_mode = S_IFDIR;
        st->st_atim.tv_sec = 0; // Time of last access
        st->st_mtim.tv_sec = 0; // Time of last modification
        st->st_ctim.tv_sec = 0; // Time of last status change
        return 0;
    }
    res = nitrofs_open_by_id(&f, res);
    if (res < 0)
    {
        errno = ENOENT;
        return -1;
    }
    return nitrofs_stat_file_internal(&f, st);
}

int nitrofs_fstat(int fd, struct stat *st)
{
    nitrofs_file_t *f = (nitrofs_file_t *) FD_DESC(fd);
    return nitrofs_stat_file_internal(f, st);
}

/// Initialization

bool nitroFSExit(void)
{
    if (nitrofs_local.fat_offset == 0)
        return true;

    if (nitrofs_local.file)
    {
        // TODO: Should we crash here if it fails? It could be leaving a file
        // descriptor open forever.
        if (fclose(nitrofs_local.file) != 0)
            return false;
    }

    nitrofs_local.fnt_offset = 0;
    nitrofs_local.fat_offset = 0;
    return true;
}

bool nitroFSInit(const char *basepath)
{
    if (nitrofs_local.fat_offset)
        nitroFSExit();

    nitrofs_local.file = NULL;
    nitrofs_local.current_dir = 0xF000;

    // Initialize cache if it hasn't been initialized already
    if (!cache_initialized())
    {
        // TODO: Should we have a cache_end() if the function fails? It may not be
        // worth it, most games would just stop booting if the filesystem isn't
        // available.
        int ret = cache_init(-1);
        if (ret != 0)
        {
            errno = ENOMEM;
            return false;
        }
    }

    // Keep track of whether the path was provided by the user or it was
    // obtained from argv.
    bool user_provided_basepath = false;

    // Use argv[0] if the user hasn't provided any path
    if (basepath == NULL)
    {
        if ((__system_argv->argvMagic == ARGV_MAGIC) && (__system_argv->argc >= 1))
        {
            basepath = __system_argv->argv[0];
        }
    }
    else
    {
        user_provided_basepath = true;
    }

    // Try to open the basepath file.
    if (basepath != NULL)
    {
        if (fatInitDefault())
        {
            nitrofs_local.file = fopen(basepath, "r");
        }

        if (nitrofs_local.file != NULL)
        {
            // If we could open the provided file, initialize the FAT lookup
            // cache for NitroFS files.
            //
            // NitroFS files inherently do a lot of seeking, so it's almost
            // always beneficial. At the same time, for a defragmented
            // drive, this should only occupy a few dozen bytes.
            //
            // FIXME: Move this to the DLDI driver space and remove the 2KB
            // size limit.
            fatInitLookupCacheFile(nitrofs_local.file, 2048);
        }
        else
        {
            // If the user provided the path and it can't be opened we need to
            // fail right away. The caller may have provided a path that isn't
            // the same file that is currently running, and all other access
            // systems can only access the same application currently running.
            if (user_provided_basepath)
            {
                // Don't set errno here, keep the one set by fatInitDefault() or
                // fopen().
                nitrofs_local.fnt_offset = 0;
                return false;
            }

            // If the path was provided by argv we can try other access modes
            // before giving up. For example, we may be running on DeSmuME,
            // which always sets argv[0] but doesn't have automatic FAT support:
            // https://github.com/TASEmulators/desmume/blob/1638bc00aeff9526601f723c81c418c303b6a04c/desmume/src/NDSSystem.cpp#L2653-L2665
        }
    }

    // This is part of the NDS header struct (tNDSHeader)
    typedef struct {
        u32 filenameOffset;
        u32 filenameSize;
        u32 fatOffset;
        u32 fatSize;
    } nitrofs_offsets_t;

    nitrofs_offsets_t nitrofs_offsets;

    // Read FNT/FAT offset/size information.
    if (nitrofs_local.file != NULL)
    {
        // If we have an open file, that's the path we need to use. This can
        // happen in two situations:
        //
        // - We are loading the same NDS currently running.
        // - The caller of nitroFSInit() has provided a different path, so we
        //   may be loading the same NDS file or a completely different one.

        nitrofs_read_internal_file(&nitrofs_offsets,
                                   offsetof(tNDSHeader, filenameOffset),
                                   sizeof(nitrofs_offsets));
    }
    else
    {
        // There is no open file:
        //
        // - We are trying to open the currently running file from an emulator,
        //   an official NDS cartridge, or a Slot-2 device.
        // - We're running on a flashcart but the NitroFS initialization failed.
        //
        // First, attempt to read from Slot-2. If that fails, attempt to read
        // from the cartridge with official cartridge commands (this should work
        // on emulators). If both fail, nitroFSInit() has failed.
        //
        // To check if either system works, we need to compare the offset table
        // currently pre-loaded in RAM with the tables that we can read from
        // Slot-2 or with the cartridge commands.

        // Reference pre-loaded offsets
        memcpy(&nitrofs_offsets, &(__NDSHeader->filenameOffset), sizeof(nitrofs_offsets));

        // Offsets that we will read
        nitrofs_offsets_t nitrofs_offsets_check;

        // If not in DSi mode and the .nds file is <= 32MB...
        if (!isDSiMode() && __NDSHeader->deviceSize <= 8)
        {
            // ... we could still be reading from Slot-2.
            // Figure this out by comparing NitroFS header data between the two.
            sysSetCartOwner(BUS_OWNER_ARM9);

            // Try to read from Slot-2
            nitrofs_read_internal_slot2(&nitrofs_offsets_check,
                                        offsetof(tNDSHeader, filenameOffset),
                                        sizeof(nitrofs_offsets_check));

            if (memcmp(&nitrofs_offsets_check, &nitrofs_offsets, sizeof(nitrofs_offsets)) == 0)
                nitrofs_local.use_slot2 = true;
            else
                nitrofs_local.use_slot2 = false;

        }

        // If we can't use Slot-2, make sure that card commands actually work
        if (!nitrofs_local.use_slot2)
        {
            nitrofs_read_internal_cart(&nitrofs_offsets_check,
                                       offsetof(tNDSHeader, filenameOffset),
                                       sizeof(nitrofs_offsets_check));

            if (memcmp(&nitrofs_offsets_check, &nitrofs_offsets, sizeof(nitrofs_offsets)) != 0)
            {
                nitrofs_local.fnt_offset = 0;
                errno = ENODEV;
                return false;
            }
        }
    }

    // Reset FNT/FAT offsets.
    nitrofs_local.fnt_offset = 0;
    nitrofs_local.fat_offset = 0;

    // Initialize FAT offset, if valid; otherwise exit.
    if (nitrofs_offsets.fatOffset >= 0x8000 && nitrofs_offsets.fatSize > 0)
    {
        nitrofs_local.fat_offset = nitrofs_offsets.fatOffset;
    }
    else
    {
        if (nitrofs_local.file)
        {
            fclose(nitrofs_local.file);
            // TODO: Should we crash here if it fails? It could be leaving a
            // file descriptor open forever.
        }

        nitrofs_local.fnt_offset = 0;
        errno = ENODEV;
        return false;
    }

    // Initialize FNT offset, if valid. Allow opening files by direct ID
    // even without an FNT.
    if (nitrofs_offsets.filenameOffset >= 0x8000 && nitrofs_offsets.filenameSize > 0)
        nitrofs_local.fnt_offset = nitrofs_offsets.filenameOffset;

    // Set "nitro:/" as default path
    current_drive_is_nitrofs = true;

    return true;
}

int nitroFSInitLookupCache(uint32_t max_buffer_size)
{
    if (!nitrofs_local.fat_offset || !nitrofs_local.file)
        return 0;
    return fatInitLookupCacheFile(nitrofs_local.file, max_buffer_size);
}
