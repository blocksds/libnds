// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef LIBNDS_NDS_ARM9_EXCEPTIONS_H__
#define LIBNDS_NDS_ARM9_EXCEPTIONS_H__

#include <stdint.h>

#include "../exceptions.h"

// The following functions are exposed to help testing libnds
int mpuRegionGet(uintptr_t addr);
bool mpuRegionIsCode(int region);

#endif // LIBNDS_NDS_ARM9_EXCEPTIONS_H__
