// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005-2008 Jason Rogers (dovoto)
// Copyright (C) 2005-2010 Dave Murphy (WinterMute)
// Copyright (C) 2005 Chris Double (doublec)

// Common types (and a few useful macros)

/// @file nds/ndstypes.h
///
/// @brief Custom types employed by libnds

#ifndef LIBNDS_NDS_NDSTYPES_H__
#define LIBNDS_NDS_NDSTYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

// Define libnds types in terms of stdint
#include <stdbool.h>
#include <stdint.h>

#define LIBNDS_STRINGIFY(x) LIBNDS_STRINGIFY_(x)
#define LIBNDS_STRINGIFY_(x) #x

#ifdef __INTELLISENSE__

#define ITCM_CODE
#define ITCM_FUNC(x) x
#define ITCM_DATA
#define ITCM_DATA_VAR(x) x
#define ITCM_BSS
#define ITCM_BSS_VAR(x) x
#define DTCM_DATA
#define DTCM_DATA_VAR(x) x
#define DTCM_BSS
#define DTCM_BSS_VAR(x) x
#define TWL_CODE
#define TWL_FUNC(x) x
#define TWL_DATA
#define TWL_DATA_VAR(x) x
#define TWL_BSS
#define TWL_BSS_VAR(x) x
#define ARM_CODE
#define THUMB_CODE
#define ALIGN(m)
#define PACKED
#define packed_struct
#define LIBNDS_DEPRECATED
#define LIBNDS_NOINLINE
#define LIBNDS_ALWAYS_INLINE
#define LIBNDS_NORETURN
#define LIBNDS_PRINTFLIKE(x, y)
#define LIBNDS_NONNULL(...)
#define COMPILER_MEMORY_BARRIER()
#define WARN_UNUSED_RESULT

#else // __INTELLISENSE__


/// Used to place a function in ITCM
#define ITCM_CODE __attribute__((__section__(".itcm.text." __FILE__ "." LIBNDS_STRINGIFY(__LINE__)), __long_call__))
/// Used to place a function in ITCM
#define ITCM_FUNC(x) __attribute__((__section__(".itcm.text." LIBNDS_STRINGIFY(x)))) x

/// Used to place initialized data in ITCM
#define ITCM_DATA __attribute__((__section__(".itcm.data." __FILE__ "." LIBNDS_STRINGIFY(__LINE__))))
/// Used to place initialized data in ITCM
#define ITCM_DATA_VAR(x) __attribute__((__section__(".itcm.data." LIBNDS_STRINGIFY(x)))) x

/// Used to place uninitialized data in ITCM
#define ITCM_BSS  __attribute__((__section__(".itcm.bss." __FILE__ "." LIBNDS_STRINGIFY(__LINE__))))
/// Used to place uninitialized data in ITCM
#define ITCM_BSS_VAR(x) __attribute__((__section__(".itcm.bss." LIBNDS_STRINGIFY(x)))) x

/// Used to place initialized data in DTCM
#define DTCM_DATA __attribute__((__section__(".dtcm." __FILE__ "." LIBNDS_STRINGIFY(__LINE__))))
/// Used to place uninitialized data in DTCM
#define DTCM_BSS __attribute__((__section__(".sbss." __FILE__ "." LIBNDS_STRINGIFY(__LINE__))))
/// Used to place initialized data in DTCM
#define DTCM_DATA_VAR(x) __attribute__((__section__(".dtcm." LIBNDS_STRINGIFY(x)))) x
/// Used to place uninitialized data in DTCM
#define DTCM_BSS_VAR(x) __attribute__((__section__(".sbss." LIBNDS_STRINGIFY(x)))) x

/// Used to place a function in DSi RAM.
#define TWL_CODE __attribute__((__section__(".twl.text." __FILE__ "." LIBNDS_STRINGIFY(__LINE__))))
/// Used to place initialized data in DSi RAM.
#define TWL_DATA __attribute__((__section__(".twl.data." __FILE__ "." LIBNDS_STRINGIFY(__LINE__))))
/// Used to place uninitialized data in DSi RAM.
#define TWL_BSS __attribute__((__section__(".twl_bss." __FILE__ "." LIBNDS_STRINGIFY(__LINE__))))
/// Used to place a function in DSi RAM.
#define TWL_FUNC(x) __attribute__((__section__(".twl.text." LIBNDS_STRINGIFY(x)))) x
/// Used to place initialized data in DSi RAM.
#define TWL_DATA_VAR(x) __attribute__((__section__(".twl.data." LIBNDS_STRINGIFY(x)))) x
/// Used to place uninitialized data in DSi RAM.
#define TWL_BSS_VAR(x) __attribute__((__section__(".twl_bss." LIBNDS_STRINGIFY(x)))) x

/// Used to tell the compiler to compile a function as ARM code
#define ARM_CODE __attribute__((__target__("arm")))
/// Used to tell the compiler to compile a function as Thumb code (default)
#define THUMB_CODE __attribute__((__target__("thumb")))

/// Aligns a struct (and other types) to "m".
#define ALIGN(m) __attribute__((__aligned__(m)))

/// Packs a struct so it won't include padding bytes.
#define PACKED __attribute__ ((__packed__))
#define packed_struct struct PACKED

/// Used to mark functions and types as deprecated.
#define LIBNDS_DEPRECATED __attribute__((__deprecated__))

/// Used to mark functions that mustn't be inlined.
#define LIBNDS_NOINLINE __attribute__((__noinline__))

/// Used to mark functions that must be inlined.
#define LIBNDS_ALWAYS_INLINE __attribute__((__always_inline__))

/// Used to mark functions that don't return to the caller.
#define LIBNDS_NORETURN __attribute__((__noreturn__))

/// Used to mark functions that expect strings formatted like printf().
#define LIBNDS_PRINTFLIKE(x, y) __attribute__((__format__(printf, x, y)))

/// Used to tag functions with arguments that can never be NULL
#define LIBNDS_NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))

/// Helper that prevents the compiler from reordering memory accesses.
///
/// Accesses to pointers marked as "volatile" aren't reordered, but accesses
/// to non-volatile pointers can be reordered. Sometimes, this reordering can
/// cause issues. For example, if you want to setup VRAM as LCD to write to it,
/// but the compiler decides to set it to LCD after writing data to it. It can
/// also happen if the compiler sets VRAM to texture mode or extended palette
/// mode before it has finished writing data to it.
///
/// This barrier makes sure that all the accesses before it happen before all
/// accesses after it.
///
/// Note that this doesn't compile to any instruction, it's just an indication
/// for the compiler.
#define COMPILER_MEMORY_BARRIER() asm volatile("" ::: "memory")

/// Makes the compiler output a warning if the return value of a function is
/// ignored.
#define WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))

#endif // __INTELLISENSE__

// Macros related to the bin2o macro of the Makefile
#define GETRAW(name)        (name)
#define GETRAWSIZE(name)    ((int)name##_size)
#define GETRAWEND(name)     ((int)name##_end)

/// Returns a number with the nth bit set.
#define BIT(n) (1u << (n))

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

LIBNDS_DEPRECATED typedef void (* IntFn)(void);
LIBNDS_DEPRECATED typedef void (* fp)(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_NDSTYPES_H__
