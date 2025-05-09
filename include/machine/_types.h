// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka

#ifndef MACHINE__TYPES_H__
#define MACHINE__TYPES_H__

// Define some platform-specific types for newlib/picolibc.

#include <stdint.h>
#include <machine/_default_types.h>

typedef uint32_t __ino_t;
typedef uint64_t __ino64_t;
#define __machine_ino_t_defined

typedef int32_t _off_t;
#define __machine_off_t_defined

#endif
