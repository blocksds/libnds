// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005-2008 Jason Rogers (Dovoto)
// Copyright (C) 2005-2010 Dave Murphy (WinterMute)
// Copyright (C) 2005 Chris Double (doublec)

// Common types (and a few useful macros)

/// @file nds/ndstypes.h
///
/// @brief Custom types employed by libnds

#ifndef LIBNDS_NDS_NDSTYPES_H__
#define LIBNDS_NDS_NDSTYPES_H__

// Define libnds types in terms of stdint
#include <stdbool.h>
#include <stdint.h>

/// Used to place a function in ITCM
#define ITCM_CODE __attribute__((section(".itcm"), long_call))

/// Used to place initialized data in DTCM
#define DTCM_DATA __attribute__((section(".dtcm")))
/// Used to place uninitialized data in DTCM
#define DTCM_BSS __attribute__((section(".sbss")))

/// Used to place a function in DSi RAM.
#define TWL_CODE __attribute__((section(".twl")))
/// Used to place initialized data in DSi RAM.
#define TWL_DATA __attribute__((section(".twl")))
/// Used to place uninitialized data in DSi RAM.
#define TWL_BSS __attribute__((section(".twl_bss")))

/// Used to tell the compiler to compile a function as ARM code
#define ARM_CODE __attribute__((target("arm")))
/// Used to tell the compiler to compile a function as Thumb code (default)
#define THUMB_CODE __attribute__((target("thumb")))

/// Aligns a struct (and other types) to "m".
#define ALIGN(m) __attribute__((aligned (m)))

/// Packs a struct so it won't include padding bytes.
#define PACKED __attribute__ ((packed))
#define packed_struct struct PACKED

// Macros related to the bin2o macro of the Makefile
#define GETRAW(name)        (name)
#define GETRAWSIZE(name)    ((int)name##_size)
#define GETRAWEND(name)     ((int)name##_end)

/// Returns a number with the nth bit set.
#define BIT(n) (1 << (n))

// Deprecated types. TODO: Remove
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef float float32;
typedef double float64;
typedef volatile float32 vfloat32;
typedef volatile float64 vfloat64;
typedef uint8_t byte;

/// 8 bit volatile unsigned integer.
typedef volatile uint8_t vuint8;
/// 16 bit volatile unsigned integer.
typedef volatile uint16_t vuint16;
/// 32 bit volatile unsigned integer.
typedef volatile uint32_t vuint32;
/// 64 bit volatile unsigned integer.
typedef volatile uint64_t vuint64;

/// 8 bit volatile signed integer.
typedef volatile int8_t vint8;
/// 16 bit volatile signed integer.
typedef volatile int16_t vint16;
/// 32 bit volatile signed integer.
typedef volatile int32_t vint32;
/// 64 bit volatile signed integer.
typedef volatile int64_t vint64;

/// 8 bit unsigned integer.
typedef uint8_t u8;
/// 16 bit unsigned integer.
typedef uint16_t u16;
/// 32 bit unsigned integer.
typedef uint32_t u32;
/// 64 bit unsigned integer.
typedef uint64_t u64;

/// 8 bit signed integer.
typedef int8_t s8;
/// 16 bit signed integer.
typedef int16_t s16;
/// 32 bit signed integer.
typedef int32_t s32;
/// 64 bit signed integer.
typedef int64_t s64;

/// 8 bit volatile unsigned integer.
typedef volatile u8 vu8;
/// 16 bit volatile unsigned integer.
typedef volatile u16 vu16;
/// 32 bit volatile unsigned integer.
typedef volatile u32 vu32;
/// 64 bit volatile unsigned integer.
typedef volatile u64 vu64;

/// 8 bit volatile signed integer.
typedef volatile s8 vs8;
/// 16 bit volatile signed integer.
typedef volatile s16 vs16;
/// 32 bit volatile signed integer.
typedef volatile s32 vs32;
/// 64 bit volatile signed integer.
typedef volatile s64 vs64;

typedef uint32_t sec_t;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// Handy function pointer typedefs

/// Function pointer that takes no arguments and doesn't return anything.
typedef void (* VoidFn)(void);

typedef void (* IntFn)(void);
typedef void (* fp)(void);

#endif // LIBNDS_NDS_NDSTYPES_H__
