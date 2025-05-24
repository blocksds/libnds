// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Antonio Niño Díaz

#ifndef LIBNDS_DLFCN_H__
#define LIBNDS_DLFCN_H__

/// @file dlfcn.h
///
/// @brief Helpers to load dynamic libraries.
///
/// The functions in this file allow the user to load DSL (Nintendo DS Loadable)
/// files. They are a simplified version of ELF files created by `dsltool`,
/// which is a tool included in BlocksDS.

#ifndef ARM9
#error dlfcn.h is currently supported on the ARM9 only.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// Perform lazy binding. Not supported.
#define RTLD_LAZY       0x01
/// Load everything right away.
#define RTLD_NOW        0x02

/// Make symbols available for other dynamic libraries. Not supported.
#define RTLD_GLOBAL     0x04

/// Make symbols only available to this dynamic libraries. Default setting.
#define RTLD_LOCAL      0x08

// Not supported.
#define RTLD_NODELETE   0x10
#define RTLD_NOLOAD     0x20
#define RTLD_DEEPBIND   0x40

/// Loads a dynamic library (in DSL format) into RAM.
///
/// @note
///     The value of the environment variable LD_LIBRARY_PATH is ignored.
///
/// @param file
///     Path of the DSL file.
/// @param mode
///     Mode in which the file will be opened. Currently, the only mode
///     supported is `RTLD_NOW | RTLD_LOCAL`. Also, RTLD_LOCAL is the default
///     setting, so it isn't required to specify it explicitly.
///
/// @return
///     On success, it returns a handle to be used by dlsym() and dlclose(). On
///     error, it returns NULL, and the user is expected to call dlerror() to
///     get a user-readable string with the reason of the error.
void *dlopen(const char *file, int mode);

/// Frees all memory used by a dynamic library.
///
/// @param handle
///     The handle returned by dlopen().
///
/// @return
///     On success, it returns 0. On error it returns a non-zero value, and the
///     user is expected to call dlerror() to get a user-readable string with
///     the reason of the error.
int dlclose(void *handle);

/// Returns a user-readable error string.
///
/// It clears the error string after being called.
///
/// @return
///     A user-readable error string or NULL if no error has happened since the
///     last call to dlerror().
char *dlerror(void);

// Not supported.
#define RTLD_NEXT       ((void *)-1)
#define RTLD_DEFAULT    ((void *)0)

/// Returns a pointer to the requested symbol.
///
/// @warning
///     Don't use free() with the pointers returned by this function.
///
/// @param handle
///     The handle returned by dlopen().
/// @param name
///     The name of the requested symbol.
///
/// @return
///     On success, it returns a pointer to the location of the symbol in
///     memory.  On error it returns a non-zero value, and the user is expected
///     to call dlerror() to get a user-readable string with the reason of the
///     error.
void *dlsym(void *handle, const char *name);

/// Returns a pointer to the base address of the code loaded with the library.
///
/// The purpose of this function is to print this address to be used when
/// debugging the code with emulators. You must provide this address when
/// loading the elf file of the library. For example, with the GDB command line:
///
/// Note that this is a libnds-specific function.
///
/// ```
/// add-symbol-file path/to/dynamic.elf -s .progbits <load_address>
/// ```
///
/// @param handle
///     The handle returned by dlopen().
///
/// @return
///     On success, it returns a pointer to the base address of the code loaded
///     to RAM. On error it returns NULL, and the user is expected to call
///     dlerror() to get a user-readable string with the reason of the error.
void *dlmembase(void *handle);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_DLFCN_H__
