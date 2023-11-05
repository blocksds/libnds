// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz
// Copyright (c) 2023 Adrian "asie" Siekierka

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <fat.h>
#include <nds/arm9/card.h>
#include <nds/card.h>
#include <nds/memory.h>
#include <nds/system.h>

// "dirent.h" defines DIR, but "ff.h" defines a different non-standard one.
// Functions in this file need to use their standard prototypes, so it is needed
// to somehow rename the DIR of "ff.h". It's better to keep the original header
// unmodified so that updating it is easier, so this is a hack to rename it just
// in this compilation unit.
#define DIR DIRff
#include "fatfs/ff.h"
#include "fatfs_internal.h"
#undef DIR
#include "filesystem_internal.h"
#include "nitrofs_internal.h"

// Include "dirent.h" after the FatFs inclusion hack.
#include <dirent.h>

static bool nitrofs_initialized = false;
static nitrofs_t nitrofs_local;
bool nitrofs_reader_is_arm9 = false;

#define MAX_NESTED_SUBDIRS 128

/// Helper functions

bool nitrofs_use_for_path(const char *path)
{
    if (strstr(path, ":/"))
        return !memcmp(path, "nitro:/", 7);
    else
        return current_drive_is_nitrofs;
}

static void nitroFSRefreshBusOwner(void)
{
    int bus_owner = nitrofs_reader_is_arm9 ? BUS_OWNER_ARM9 : BUS_OWNER_ARM7;
    sysSetBusOwners(bus_owner, bus_owner);
}

#define NDS_CARD_BLOCK_SIZE 0x200 // CARD_BLK_SIZE(1)

void cardReadUnaligned(void *dest, size_t offset, size_t len)
{
    uint8_t buffer[NDS_CARD_BLOCK_SIZE] __attribute__((aligned(4)));
    uint8_t *pc = dest;

    while (len)
    {
        // are both the read offset and the destination buffer word-aligned?
        if (!(offset & 3) && !(((uint32_t) pc) & 3) && len >= 4)
        {
            // fast direct read
            if (nitrofs_reader_is_arm9)
                cardRead(pc, offset, len & (~3));
            else
                cardReadArm7(pc, offset, len & (~3));
            
            pc += (len & (~3));
            len &= 3;

            if (!len) break;
        }

        // slow buffered read
        size_t block_offset = (offset & 3); // offset from word alignment
        size_t block_len = len;

        // adjust offset to have word alignment
        if (block_offset)
        {
            block_len += 4 - block_offset;
            offset -= block_offset;
        }

        // adjust block_len to have word alignment
        if (block_len > NDS_CARD_BLOCK_SIZE)
            block_len = NDS_CARD_BLOCK_SIZE;
        size_t block_len_aligned = (block_len + 3) & ~3;

        // the length of data actually written to dest
        size_t dest_block_len = block_len - block_offset;

        if (nitrofs_reader_is_arm9)
            cardRead(buffer, offset, block_len_aligned);
        else
            cardReadArm7(buffer, offset, block_len_aligned);

        memcpy(pc, buffer + block_offset, dest_block_len);
        offset += block_len;
        pc += dest_block_len;
        len -= dest_block_len;
    }
}

static ssize_t nitrofs_read_internal(void *ptr, size_t offset, size_t len)
{
    if (nitrofs_local.file)
    {
        fseek(nitrofs_local.file, offset, SEEK_SET);
        return fread(ptr, 1, len, nitrofs_local.file);
    }
    else
    {
        nitroFSRefreshBusOwner();
        cardReadUnaligned(ptr, offset, len);
        return len;
    }
}

/// Directory I/O

static bool nitrofs_dir_state_init(nitrofs_dir_state_t *state, uint16_t dir)
{
    nitrofs_fnt_entry_t fnt_entry;

    nitrofs_read_internal(&fnt_entry, nitrofs_local.fnt_offset + ((dir - 0xF000) * 8), sizeof(fnt_entry));
    state->offset = nitrofs_local.fnt_offset + fnt_entry.offset;
    state->position = 0;
    state->file_index = fnt_entry.first_file;
    state->dir_opened = dir;
    state->buffer[0] = 0;
    nitrofs_read_internal(state->buffer, state->offset, 512);
    return state->buffer[0] != 0;
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
        if ((state->position + length) >= 512)
        {
            uint32_t shift = state->position & ~3;
            uint32_t next_sector_offset = 512 - shift;
            memcpy(state->buffer, state->buffer + shift, next_sector_offset);
            state->offset += 512;
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
        if (len == name_len && !memcmp(name, (const char*) (state.buffer + state.position + 1), len))
            return nitrofs_dir_state_index(&state);
    } while (nitrofs_dir_state_next(&state));

    return -1;
}

int nitrofs_opendir(nitrofs_dir_state_t *state, const char *name)
{
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
    uint8_t type = state->buffer[state->position];

    size_t len = type & 0x7F;
    if (len == 0)
        return -1;
    if (len > sizeof(ent->d_name))
        len = sizeof(ent->d_name);
    strncpy(ent->d_name, (const char*) (state->buffer + state->position + 1), len);
    ent->d_name[sizeof(ent->d_name) - 1] = '\0';

    ent->d_type = (type & 0x80) ? DT_DIR : DT_REG;

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

    char *sep = (char*) path;
    while (*sep)
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

    // string is finalized by parent getcwd()
    return 0;
}

int nitrofs_chdir(const char *path)
{
    int32_t res = nitrofs_path_resolve((char*) path);
    if (res < 0) return FR_NO_PATH;
    nitrofs_local.current_dir = res;
    return FR_OK;
}

/// File I/O

ssize_t nitrofs_read(int fd, void *ptr, size_t len)
{
    if (!nitrofs_initialized)
        return 0;

    nitrofs_file_t *fp = (nitrofs_file_t*) FD_DESC(fd);
    size_t remaining = fp->endofs - fp->position;
    if (len > remaining)
        len = remaining;
    if (len == 0)
        return 0;
    ssize_t result = nitrofs_read_internal(ptr, fp->position, len);
    if (result <= 0)
        return result;
    fp->position += result;
    return result;
}

off_t nitrofs_lseek(int fd, off_t offset, int whence)
{
    nitrofs_file_t *fp = (nitrofs_file_t*) FD_DESC(fd);
    size_t new_position;

    if (whence == SEEK_END)
        new_position = fp->endofs + offset;
    else if (whence == SEEK_CUR)
        new_position = fp->position + offset;
    else if (whence == SEEK_SET)
        new_position = fp->offset + offset;
    else
    {
        errno = EINVAL;
        return (off_t)-1;
    }

    if (new_position < fp->offset)
        new_position = fp->offset;
    else if (new_position > fp->endofs)
        new_position = fp->endofs;
    fp->position = new_position;
    return new_position - fp->offset;
}

int nitrofs_close(int fd)
{
    nitrofs_file_t *fp = (nitrofs_file_t*) FD_DESC(fd);
    free(fp);
    return 0;
}

static int nitrofs_open_by_id(nitrofs_file_t *fp, uint16_t id)
{
    if (id >= 0xF000)
    {
        // not a file
        return -1;
    }
    nitrofs_read_internal(fp, nitrofs_local.fat_offset + (id * 8), 8);
    fp->position = fp->offset;
    return 0;
}

int nitrofs_open(const char *name)
{
    int32_t res = nitrofs_path_resolve(name);
    if (res < 0)
    {
        errno = ENOENT;
        return -1;
    }
    nitrofs_file_t *fp = malloc(sizeof(nitrofs_file_t));
    if (fp == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    res = nitrofs_open_by_id(fp, res);
    if (res < 0)
    {
        free(fp);
        errno = ENOENT;
        return -1;
    }
    return FD_DESC(fp) | (FD_TYPE_NITRO << 28);
}

static int nitrofs_stat_file_internal(nitrofs_file_t *fp, struct stat *st)
{
    st->st_size = fp->endofs - fp->offset;
    st->st_blksize = NDS_CARD_BLOCK_SIZE;
    st->st_blocks = (st->st_size + NDS_CARD_BLOCK_SIZE - 1) / NDS_CARD_BLOCK_SIZE;
    st->st_mode = S_IFREG;

    return 0;
}

int nitrofs_stat(const char *name, struct stat *st)
{
    nitrofs_file_t fp;
    int32_t res = nitrofs_path_resolve(name);
    if (res < 0)
    {
        errno = ENOENT;
        return -1;
    }
    else if (res >= 0xF000)
    {
        st->st_size = 0;
        st->st_mode = S_IFDIR;
        return 0;
    }
    res = nitrofs_open_by_id(&fp, res);
    if (res < 0)
    {
        errno = ENOENT;
        return -1;
    }
    return nitrofs_stat_file_internal(&fp, st);
}

int nitrofs_fstat(int fd, struct stat *st)
{
    nitrofs_file_t *fp = (nitrofs_file_t*) FD_DESC(fd);
    return nitrofs_stat_file_internal(fp, st);
}

/// Initialization

void nitroFSExit(void)
{
    if (nitrofs_initialized)
    {
        if (nitrofs_local.file)
            fclose(nitrofs_local.file);

        nitrofs_initialized = false;
    }
}

bool nitroFSInit(const char *basepath)
{
    uint32_t nitrofs_info[4];

    if (nitrofs_initialized)
        nitroFSExit();

    nitrofs_local.file = NULL;
    nitrofs_local.current_dir = 0xF000;

    // Use argv[0] if basepath is not provided.
    if (!basepath && __system_argv->argvMagic == ARGV_MAGIC && __system_argv->argc >= 1)
        basepath = __system_argv->argv[0];

    // Try to open the basepath file.
    if (basepath)
    {
        fatInitDefault();
        nitrofs_local.file = fopen(basepath, "r");
        if (!nitrofs_local.file)
            basepath = NULL;
    }

    // Read FNT/FAT offset
    memset(nitrofs_info, 0, sizeof(nitrofs_info));
    nitrofs_read_internal(nitrofs_info, 0x40, sizeof(nitrofs_info));
    if (!(nitrofs_info[0] >= 0x200 && nitrofs_info[2] >= 0x200))
        return false;
    nitrofs_local.fnt_offset = nitrofs_info[0];
    nitrofs_local.fat_offset = nitrofs_info[2];
    
    // Set "nitro:/" as default path
    current_drive_is_nitrofs = true;
    nitrofs_local.current_dir = 0;

    nitrofs_initialized = true;

    return true;
}

void nitroFSSetReaderCPU(bool use_arm9)
{
    nitrofs_reader_is_arm9 = use_arm9;

    nitroFSRefreshBusOwner();
}