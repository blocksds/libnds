// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBTEAK_TYPES_H__
#define LIBTEAK_TYPES_H__

#include <stdint.h>

#define BIT(n) (1 << (n))

/// Aligns struct, arrays, etc, to "m".
#define ALIGN(m) __attribute__((aligned (m)))

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

#endif // LIBTEAK_TYPES_H__
