// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Antonio Niño Díaz

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nds/arm9/cache.h>
#include <nds/exceptions.h>
#include <nds/ndstypes.h>

#include "dsl.h"

// This holds a pointer to the last error caused by the functions in this file.
// All threads have their own pointer.
static __thread char *dl_err_str = NULL;

typedef struct {
    void (*fn) (void *);
    void *arg;
} dsl_dtor;

// This is the internal structure of a handle returned by dlopen().
typedef struct {
    void *loaded_mem;
    dsl_symbol_table *sym_table;

    dsl_dtor *dtors_list;
    int dtors_num;
    int dtors_max;
} dsl_handle;

// Some ELF-related definitions

// Check the following link for information about the relocations:
// https://github.com/ARM-software/abi-aa/blob/9498b4eef7b3616fafeab15bf6891ab365a071be/aaelf32/aaelf32.rst

typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Word;

typedef struct {
    Elf32_Addr r_offset; // Location (virtual address)
    Elf32_Word r_info;   // (symbol table index << 8) | (type of relocation)
} Elf32_Rel;

#define R_ARM_NONE          0
#define R_ARM_ABS32         2
#define R_ARM_REL32         3
#define R_ARM_THM_CALL      10
#define R_ARM_BASE_PREL     25
#define R_ARM_GOT_BREL      26
#define R_ARM_CALL          28
#define R_ARM_JUMP24        29
#define R_ARM_THM_JUMP24    30
#define R_ARM_TARGET1       38
#define R_ARM_TLS_IE32      107
#define R_ARM_TLS_LE32      108

// Size of a thread control block. TLS relocations are generated relative to a
// location before tdata and tbss.
#define TCB_SIZE 8

// While the constructors of a library are being called, this pointer holds the
// address of the handle being loaded.
static __thread dsl_handle *dsl_current = NULL;

// fini_array isn't really used by global destructors. Instead, global
// constructors call __aeabi_atexit() so that the destructors are called in the
// opposite order of the constructors. Also, in case a global constructor isn't
// called, the destructor won't be called either. More information here:
// https://etherealwake.com/2021/09/crt-startup/#c-abi-extensions
__attribute__((noinline))
int __aeabi_atexit(void *arg, void (*func) (void *), void *dso_handle)
{
    (void)dso_handle;

    if (func == NULL)
        return -1;

    if (dsl_current == NULL)
    {
        // TODO: Should this just call atexit()?
        libndsCrash("Unexpected call to __aeabi_atexit()");
    }
    else
    {
        dsl_handle *handle = dsl_current;

        if (handle->dtors_num == handle->dtors_max)
            libndsCrash("Too many dtors in dynamic lib");

        handle->dtors_list[handle->dtors_num].fn = func;
        handle->dtors_list[handle->dtors_num].arg = arg;

        handle->dtors_num++;
    }

    return 0;
}

void *dlopen(const char *file, int mode)
{
    // Clear error string
    dl_err_str = NULL;

    FILE *f = NULL;
    uint8_t *loaded_mem = NULL;
    dsl_handle *handle = NULL;
    dsl_symbol_table *sym_table = NULL;

    int unsupported_mask = RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE | RTLD_NOLOAD
                         | RTLD_DEEPBIND;

    if (mode & unsupported_mask)
    {
        dl_err_str = "unsupported mode parameter";
        return NULL;
    }

    if ((file == NULL) || (strlen(file) == 0))
    {
        dl_err_str = "no file provided";
        return NULL;
    }

    // RTLD_NOW or RTLD_LAZY need to be set, but only RTLD_NOW is supported.
    if ((mode & RTLD_NOW) == 0)
    {
        dl_err_str = "RTLD_NOW mode required";
        return NULL;
    }

    // RTLD_LOCAL is the default setting, but it doesn't need to be set
    // manually.

    f = fopen(file, "rb");
    if (f == NULL)
    {
        dl_err_str = "file can't be opened";
        return NULL;
    }

    uint8_t num_sections;
    uint32_t addr_space_size;

    {
        dsl_header header;
        if (fread(&header, sizeof(header), 1, f) != 1)
        {
            dl_err_str = "can't read DSL header";
            goto cleanup;
        }

        if ((header.magic != DSL_MAGIC) || (header.version != 0))
        {
            dl_err_str = "invalid DSL magic or version";
            goto cleanup;
        }

        num_sections = header.num_sections;
        addr_space_size = header.addr_space_size;
    }

    loaded_mem = malloc(addr_space_size);
    if (loaded_mem == NULL)
    {
        dl_err_str = "no memory to load sections";
        goto cleanup;
    }

    dsl_section_header section[10];
    if (num_sections > 10)
    {
        dl_err_str = "too many sections";
        goto cleanup;
    }

    if (fread(&section, sizeof(dsl_section_header), num_sections, f) != num_sections)
    {
        dl_err_str = "can't read DSL sections";
        goto cleanup;
    }

    // Load progbits sections and clear nobits sections. Skip relocations.
    for (unsigned int i = 0; i < num_sections; i++)
    {
        uintptr_t address = section[i].address;
        size_t size = section[i].size;
        int type = section[i].type;
        size_t data_offset = section[i].data_offset;

        if (type == DSL_SEGMENT_NOBITS)
        {
            memset(loaded_mem + address, 0, size);
            continue;
        }

        // Check this only for sections with data
        long int cursor = ftell(f);
        if ((size_t)cursor != data_offset)
        {
            dl_err_str = "sections not in order";
            goto cleanup;
        }

        if (type == DSL_SEGMENT_PROGBITS)
        {
            if (fread(loaded_mem + address, 1, size, f) != size)
            {
                dl_err_str = "section data can't be read";
                goto cleanup;
            }
        }
        else if (type == DSL_SEGMENT_RELOCATIONS)
        {
            // Skip section
            if (fseek(f, size, SEEK_CUR) != 0)
            {
                dl_err_str = "section data can't be skipped";
                goto cleanup;
            }
        }
    }

    // Load symbol table
    // -----------------

    long int symbol_table_start = ftell(f);

    // Calculate size of symbol table by checking the size of the file
    if (fseek(f, 0, SEEK_END) != 0)
    {
        dl_err_str = "can't seek end of file";
        goto cleanup;
    }

    long int file_size = ftell(f);

    if (fseek(f, symbol_table_start, SEEK_SET) != 0)
    {
        dl_err_str = "can't seek symbol table";
        goto cleanup;
    }

    size_t symbol_table_size = file_size - symbol_table_start;

    sym_table = malloc(symbol_table_size);
    if (sym_table == NULL)
    {
        dl_err_str = "no memory to load symbol table";
        goto cleanup;
    }

    if (fread(sym_table, symbol_table_size, 1, f) != 1)
    {
        dl_err_str = "can't read symbol table";
        goto cleanup;
    }

    // Start preparing the handle
    // --------------------------

    handle = calloc(1, sizeof(dsl_handle));
    if (handle == NULL)
    {
        dl_err_str = "no memory to create handle";
        goto cleanup;
    }

    handle->loaded_mem = loaded_mem;
    handle->sym_table = sym_table;

    // Apply relocations
    // -----------------

    for (unsigned int i = 0; i < num_sections; i++)
    {
        int type = section[i].type;

        if (type != DSL_SEGMENT_RELOCATIONS)
            continue;

        size_t size = section[i].size;
        size_t data_offset = section[i].data_offset;

        if (fseek(f, data_offset, SEEK_SET) != 0)
        {
            dl_err_str = "can't seek relocations";
            goto cleanup;
        }

        size_t num_relocs = size / sizeof(Elf32_Rel);

        for (size_t r = 0; r < num_relocs; r++)
        {
            Elf32_Rel rel;
            if (fread(&rel, sizeof(Elf32_Rel), 1, f) != 1)
            {
                dl_err_str = "can't read relocation";
                goto cleanup;
            }

            int rel_type = rel.r_info & 0xFF;
            int rel_symbol = rel.r_info >> 8;

            if ((rel_type == R_ARM_ABS32) || (rel_type == R_ARM_TARGET1))
            {
                // R_ARM_TARGET1 behaves as R_ARM_ABS32 due to the linker option
                // -Wl,--target1-abs.
                uint32_t *ptr = (uint32_t *)(loaded_mem + rel.r_offset);
                *ptr += (uintptr_t)loaded_mem;
            }
            else if (rel_type == R_ARM_THM_CALL)
            {
                dsl_symbol *sym = &(sym_table->symbol[rel_symbol]);

                // If the symbol is in the dynamic library, we don't need to do
                // anything because BL/BLX instructions are relative, and all
                // the sources and destinations are in the same section, so
                // moving the section around doesn't matter. We only need to fix
                // symbols that are in the main binary.

                if (sym->attributes & DSL_SYMBOL_MAIN_BINARY)
                {
                    // We need to adjust the branch to jump to the right symbol
                    // in the main binary. The range of BL/BLX is +/-4 MB, so
                    // it will always work if the source and destination are in
                    // main RAM.

                    uint32_t bl_addr = (uint32_t)(loaded_mem + rel.r_offset);
                    uint32_t sym_addr = sym->value;
#if 0
                    // Note: In ARM7 it isn't possible to do interworking calls
                    // (from Thumb to ARM) because BLX doesn't exist. This check
                    // will be required if we want to enable dynamic libraries
                    // on the ARM7.
                    if ((sym_addr & 1) == 0)
                    {
                        dl_err_str = "R_ARM_THM_CALL can't switch to ARM in ARMv4";
                        goto cleanup;
                    }
#endif
                    bool to_arm = false;
                    if ((sym_addr & 1) == 0)
                        to_arm = true;

                    int32_t jump_value = sym_addr - bl_addr;

                    if (to_arm)
                        jump_value -= 2;
                    else
                        jump_value -= 4;

                    if ((jump_value > 0x3FFFFF) | (jump_value <= -0x3FFFFF))
                    {
                        dl_err_str = "R_ARM_THM_CALL outside of range";
                        goto cleanup;
                    }

                    // BL/BLX is basically a relative jump with a signed offset.
                    // BL stays in Thumb mode, BLX forces a switch to ARM mode.
                    //
                    // 1111_0nnn_nnnn_nnnn
                    //      LR = PC + 4 + (nn SHL 12)
                    // 1110_1nnn_nnnn_nnn0 (BLX, ARMv5 only)
                    // 1111_1nnn_nnnn_nnnn (BL)
                    //      PC = LR + (nn SHL 1) ; LR = (PC + 2) OR 1

                    uint16_t *ptr = (uint16_t *)(loaded_mem + rel.r_offset);

                    ptr[0] = 0xF000 | (0x07FF & (jump_value >> 12));

                    if (to_arm)
                    {
                        // Switch to ARM, BLX
                        ptr[1] = 0xE800 | (0x07FE & (jump_value >> 1));
                    }
                    else
                    {
                        // Stay in Thumb, BL
                        ptr[1] = 0xF800 | (0x07FF & (jump_value >> 1));
                    }
                }
            }
            else if (rel_type == R_ARM_JUMP24)
            {
                dsl_symbol *sym = &(sym_table->symbol[rel_symbol]);

                // If the symbol is in the dynamic library, we don't need to do
                // anything because B instructions are relative, and all the
                // sources and destinations are in the same section, so moving
                // the section around doesn't matter. We only need to fix
                // symbols that are in the main binary.

                if (sym->attributes & DSL_SYMBOL_MAIN_BINARY)
                {
                    // We need to adjust the branch to jump to the right symbol
                    // in the main binary. The range of B is +/-32 MB, so it
                    // will always work if the source and destination are in
                    // main RAM.

                    uint32_t b_addr = (uint32_t)(loaded_mem + rel.r_offset);
                    uint32_t sym_addr = sym->value;

                    // The AAELF32 ABI says that a veneer is required for
                    // R_ARM_JUMP24 when switching to Thumb mode. This is a bit
                    // tricky, so let's fail in that case:
                    //
                    // https://github.com/ARM-software/abi-aa/blob/4492d1570eb70c8fd146623e0db65b2d241f12e7/aaelf32/aaelf32.rst
                    //
                    // This isn't supported in LLVM or the Linux kernel either:
                    //
                    // https://elixir.bootlin.com/linux/v6.13.1/source/arch/arm/kernel/module.c#L129-L134
                    if ((sym_addr & 1) == 1)
                    {
                        dl_err_str = "R_ARM_JUMP24 jump to Thumb";
                        goto cleanup;
                    }

                    int32_t jump_value = sym_addr - b_addr;

                    jump_value -= 6;

                    if ((jump_value > 0x7FFFFF) | (jump_value <= -0x7FFFFF))
                    {
                        dl_err_str = "R_ARM_JUMP24 outside of range";
                        goto cleanup;
                    }

                    // B stays in ARM mode, BX forces a switch to Thumb mode.
                    //
                    // B:
                    //     jump address = nnn << 2
                    //     cccc_1010_nnnn_nnnn_nnnn_nnnn_nnnn_nnnn

                    uint32_t *ptr = (uint32_t *)(loaded_mem + rel.r_offset);

                    // Stay in ARM, B{cond}

                    *ptr = (*ptr & 0xFF000000)
                         | ((jump_value >> 2) & 0x00FFFFFF);
                }
            }
            else if (rel_type == R_ARM_CALL)
            {
                dsl_symbol *sym = &(sym_table->symbol[rel_symbol]);

                // If the symbol is in the dynamic library, we don't need to do
                // anything because BL/BLX instructions are relative, and all
                // the sources and destinations are in the same section, so
                // moving the section around doesn't matter. We only need to fix
                // symbols that are in the main binary.

                if (sym->attributes & DSL_SYMBOL_MAIN_BINARY)
                {
                    // We need to adjust the branch to jump to the right symbol
                    // in the main binary. The range of BL/BLX is +/-32 MB, so
                    // it will always work if the source and destination are in
                    // main RAM.

                    uint32_t bl_addr = (uint32_t)(loaded_mem + rel.r_offset);
                    uint32_t sym_addr = sym->value;
#if 0
                    // Note: In ARM7 it isn't possible to do interworking calls
                    // (from ARM to Thumb) because BLX doesn't exist. This check
                    // will be required if we want to enable dynamic libraries
                    // on the ARM7.
                    if ((sym_addr & 1) == 0)
                    {
                        dl_err_str = "R_ARM_CALL can't switch to Thumb in ARMv4";
                        goto cleanup;
                    }
#endif
                    bool to_arm = false;
                    if ((sym_addr & 1) == 0)
                        to_arm = true;

                    int32_t jump_value = sym_addr - bl_addr;

                    if (to_arm)
                        jump_value -= 6;
                    else
                        jump_value -= 8;

                    if ((jump_value > 0x7FFFFF) | (jump_value <= -0x7FFFFF))
                    {
                        dl_err_str = "R_ARM_CALL outside of range";
                        goto cleanup;
                    }

                    // BL/BLX is basically a relative jump with a signed offset.
                    // BL stays in ARM mode, BLX forces a switch to Thumb mode.
                    //
                    // BL:
                    //     jump address = nnn << 2
                    //     cccc_1011_nnnn_nnnn_nnnn_nnnn_nnnn_nnnn
                    //
                    // BLX (ARMv5 only)
                    //
                    //     jump address = nnn << 2 | h << 1
                    //     1111_101h_nnnn_nnnn_nnnn_nnnn_nnnn_nnnn

                    uint32_t *ptr = (uint32_t *)(loaded_mem + rel.r_offset);

                    if (!to_arm)
                    {
                        // Switch to Thumb, BLX
                        *ptr = 0xFA000000
                             | ((jump_value >> 2) & 0x00FFFFFF)
                             | ((jump_value & BIT(1)) << 23);
                    }
                    else
                    {
                        // Stay in ARM, BL
                        *ptr = (*ptr & 0xFF000000)
                             | ((jump_value >> 2) & 0x00FFFFFF);
                    }
                }
            }
            else if (rel_type == R_ARM_TLS_LE32)
            {
                uint32_t *ptr = (uint32_t *)(loaded_mem + rel.r_offset);
                dsl_symbol *sym = &(sym_table->symbol[rel_symbol]);

                *ptr = sym->value + TCB_SIZE;
            }
            else
            {
                dl_err_str = "unknown relocation";
                goto cleanup;
            }
        }

        break;
    }

    fclose(f);

    // Now that we have finished loading and handling relocations we need to
    // flush the data cache. If not, the instruction cache won't see the updated
    // code in main RAM! Also, we need to clear the instruction cache in case
    // this range was already in cache because of a previous library, for
    // example.

    DC_FlushRange(loaded_mem, addr_space_size);
    IC_InvalidateRange(loaded_mem, addr_space_size);

    // After all the code is loaded check if there are any global constructors
    // and call them.

    void *__bothinit_array_start = dlsym(handle, "__bothinit_array_start");
    void *__bothinit_array_end = dlsym(handle, "__bothinit_array_end");
    dl_err_str = NULL; // Ignore errors

    handle->dtors_list = NULL;
    handle->dtors_num = 0;
    handle->dtors_max = 0;

    if ((__bothinit_array_end != NULL) && (__bothinit_array_start != NULL))
    {
        size_t num_ctors = ((uintptr_t)__bothinit_array_end -
                            (uintptr_t)__bothinit_array_start) / 4;

        // Allocate memory for destructors

        handle->dtors_list = calloc(num_ctors, sizeof(dsl_dtor));
        if (handle->dtors_list == NULL)
        {
            dl_err_str = "no memory for destructors";
            goto cleanup;
        }
        handle->dtors_max = num_ctors;

        // Call constructors

        dsl_current = handle;

        VoidFn *ctor = __bothinit_array_start;
        for (size_t i = 0; i < num_ctors; i++)
            ctor[i]();

        dsl_current = NULL;
    }

    return handle;

cleanup:
    if (f != NULL)
        fclose(f);

    if (loaded_mem != NULL)
        free(loaded_mem);

    if (sym_table != NULL)
        free(sym_table);

    if (handle != NULL)
        free(handle);

    // This is a hack to make sure that __aeabi_atexit() is always included in
    // the final binary if dlopen() is used. __aeabi_atexit() is marked as
    // "noinline", so this will force the linker to include it.
    __aeabi_atexit(NULL, NULL, NULL);

    return NULL;
}

int dlclose(void *handle)
{
    // Clear error string
    dl_err_str = NULL;

    if (handle == NULL)
    {
        dl_err_str = "invalid handle";
        return -1;
    }

    // Before freeing the library check if there are any global destructors to
    // be and call them. They must be called from end to start.

    void *__fini_array_start = dlsym(handle, "__fini_array_start");
    void *__fini_array_end = dlsym(handle, "__fini_array_end");
    dl_err_str = NULL; // Ignore errors

    if ((__fini_array_end != NULL) && (__fini_array_start != NULL))
    {
        size_t num_dtors = ((uintptr_t)__fini_array_end -
                            (uintptr_t)__fini_array_start) / 4;

        VoidFn *dtor = __fini_array_start;

        for (size_t i = 0; i < num_dtors; i++)
            dtor[num_dtors - i - 1]();
    }

    dsl_handle *h = handle;

    for (int i = 0; i < h->dtors_num; i++)
    {
        dsl_dtor *dtor = &(h->dtors_list[h->dtors_num - i - 1]);
        dtor->fn(dtor->arg);
    }

    // Free memory

    free(h->loaded_mem);
    free(h->sym_table);
    free(h->dtors_list);
    free(h);

    return 0;
}

char *dlerror(void)
{
    // Return the current error string, but clear it so that the  next call to
    // dlerror() returns NULL.

    char *curr_str = dl_err_str;

    dl_err_str = NULL;

    return curr_str;
}

void *dlsym(void *handle, const char *name)
{
    // Clear error string
    dl_err_str = NULL;

    if ((handle == RTLD_NEXT) || (handle == RTLD_DEFAULT))
    {
        dl_err_str = "invalid handle";
        return NULL;
    }

    if ((name == NULL) || (strlen(name) == 0))
    {
        dl_err_str = "invalid symbol name";
        return NULL;
    }

    char *loaded_mem = ((dsl_handle *)handle)->loaded_mem;
    dsl_symbol_table *sym_table = ((dsl_handle *)handle)->sym_table;

    for (unsigned int i = 0; i < sym_table->num_symbols; i++)
    {
        dsl_symbol *sym = &(sym_table->symbol[i]);

        const char *sym_name = sym->name_str_offset + (const char *)sym_table;

        // Only return public symbols
        if ((sym->attributes & DSL_SYMBOL_PUBLIC) == 0)
            continue;

        if (strcmp(sym_name, name) == 0)
            return sym->value + loaded_mem;
    }

    dl_err_str = "symbol not found";
    return NULL;
}

void *dlmembase(void *handle)
{
    // Clear error string
    dl_err_str = NULL;

    if (handle == NULL)
    {
        dl_err_str = "invalid handle";
        return NULL;
    }

    dsl_handle *h = handle;

    return h->loaded_mem;
}
