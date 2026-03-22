// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Antonio Niño Díaz
// Copyright (C) 2026 trustytrojan

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

#include <stdio.h>
#include <stdint.h>

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

/// Loads a dynamic library (in DSL format) into RAM from a FILE handle.
///
/// The handle must be opened before dlopen_FILE() is called and it must be
/// closed after dlopen_FILE() returns. It isn't needed after the library has
/// been loaded.
///
/// For example, you can use this to load a dynamic library that you have
/// already loaded to RAM. You can use `fmemopen()` on the buffer where the
/// library is stored and pass the resulting handle to `dlopen_FILE()`.
///
/// @note
///     This is a non-standard function, it's only available in libnds.
///
/// @note
///     The value of the environment variable LD_LIBRARY_PATH is ignored.
///
/// @param f
///     Open file handle to the DSL library file.
/// @param mode
///     Mode in which the file will be opened. Currently, the only mode
///     supported is `RTLD_NOW | RTLD_LOCAL`. Also, RTLD_LOCAL is the default
///     setting, so it isn't required to specify it explicitly.
///
/// @return
///     On success, it returns a handle to be used by dlsym() and dlclose(). On
///     error, it returns NULL, and the user is expected to call dlerror() to
///     get a user-readable string with the reason of the error.
void *dlopen_FILE(FILE *f, int mode);

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
/// @note
///     This is a non-standard function, it's only available in libnds.
///
/// @param handle
///     The handle returned by dlopen().
///
/// @return
///     On success, it returns a pointer to the base address of the code loaded
///     to RAM. On error it returns NULL, and the user is expected to call
///     dlerror() to get a user-readable string with the reason of the error.
void *dlmembase(void *handle);

typedef bool (*SymbolResolverFn)(const char *name, uint32_t *value, uint32_t attributes);

/// Set the symbol resolver callback function. Passing `NULL` is allowed, and disables symbol resolution.
/// You may provide a callback to resolve (or override) symbol values of a DSL before relocation.
/// This is the primary way to let DSLs call functions in each other.
///
/// The callback must return whether symbol resolution succeeded as a boolean.
/// If `false` is returned for any symbol, `dlopen()` will fail with the error "symbol resolver failed".
///
/// You should use this safety mechanism to return `false` if you did not resolve a symbol attributed with `DSL_SYMBOL_UNRESOLVED`.
/// Not resolving these symbols while returning `true` will result in undefined behavior.
///
/// @note
///     This is a non-standard function, it's only available in libnds.
///
/// @param fn
///     The symbol resolver callback function.
void dsl_set_symbol_resolver(SymbolResolverFn fn);

// DSL symbol attributes.
#define DSL_SYMBOL_PUBLIC       1 ///< If not set, the symbol is private
#define DSL_SYMBOL_MAIN_BINARY  2 ///< If set, the symbol is in the main binary
#define DSL_SYMBOL_UNRESOLVED   4 ///< If set, the symbol must be resolved at runtime

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_DLFCN_H__
