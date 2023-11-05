// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_FILESYSTEM_H__
#define LIBNDS_FILESYSTEM_H__

/// @file filesystem.h
///
/// @brief NitroFS filesystem embedded in a NDS ROM.

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/// Initializes NitroFS.
///
/// If basepath or argv[0] is provided, an attempt to read the file system
/// using DLDI will be made first. If that fails, the official cartridge
/// protocol will be used instead.
///
/// @param basepath The .nds file path - NULL to auto-detect.
/// @return It returns true on success, false on error.
bool nitroFSInit(const char *basepath);

/// Exits NitroFS.
bool nitroFSExit(void);

/// Select the CPU that will do the NitroFS reads.
///
/// When NitroFS detects it's running in an official cartridge, or in an
/// emulator, this function lets you define the CPU that is in charge of reading
/// from the cart. By default the ARM7 is in charge of reading the cart. This
/// function lets you switch to using the ARM9 as well. You may switch between
/// CPUs at runtime, but be careful to not switch while the card is being read.
///
/// @param use_arm9 Set to true to use the ARM9, false to use the ARM7.
void nitroFSSetReaderCPU(bool use_arm9);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FILESYSTEM_H__
