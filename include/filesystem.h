// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_FILESYSTEM_H__
#define LIBNDS_FILESYSTEM_H__

/// @file filesystem.h
///
/// @brief NitroFAT, filesystem embedded in a NDS ROM (mostly compatible with
/// NitroFS).

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/// Initializes NitroFAT.
///
/// If argv[0] has been set to a non-NULL string, it will call fatInitDefault()
/// internally.
///
/// This function can be called multiple times, only the first one has any
/// effect. Any call after the first one returns the value returned the first
/// time.
///
/// @param basepath Ignored. Please, always pass NULL to it.
/// @return It returns true on success, false on error.
bool nitroFSInit(char **basepath);

/// Select the CPU that will do the NitroFAT reads.
///
/// When NitroFAT detects it's running in an official cartridge, or in an
/// emulator, this function lets you define the CPU that is in charge of reading
/// from the cart. By default the ARM7 is in charge of reading the cart. This
/// function lets you switch to using the ARM9 as well. You may switch between
/// CPUs at runtime, but be careful to not switch while the card is being read.
///
/// @param use_arm9 Set to true to use the ARM9, false to use the ARM7.
void nitroFATSetReaderCPU(bool use_arm9);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_FILESYSTEM_H__
