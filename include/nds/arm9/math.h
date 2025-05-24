// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/arm9/math.h
///
/// @brief hardware coprocessor math instructions.
///
/// @warning Only one type of sqrt and one type of division can be used
/// concurrently.

#ifndef LIBNDS_NDS_ARM9_MATH_H__
#define LIBNDS_NDS_ARM9_MATH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

#define REG_DIVCNT          (*(vu16 *)(0x04000280))
#define REG_DIV_NUMER       (*(vs64 *)(0x04000290))
#define REG_DIV_NUMER_L     (*(vs32 *)(0x04000290))
#define REG_DIV_NUMER_H     (*(vs32 *)(0x04000294))
#define REG_DIV_DENOM       (*(vs64 *)(0x04000298))
#define REG_DIV_DENOM_L     (*(vs32 *)(0x04000298))
#define REG_DIV_DENOM_H     (*(vs32 *)(0x0400029C))
#define REG_DIV_RESULT      (*(vs64 *)(0x040002A0))
#define REG_DIV_RESULT_L    (*(vs32 *)(0x040002A0))
#define REG_DIV_RESULT_H    (*(vs32 *)(0x040002A4))
#define REG_DIVREM_RESULT   (*(vs64 *)(0x040002A8))
#define REG_DIVREM_RESULT_L (*(vs32 *)(0x040002A8))
#define REG_DIVREM_RESULT_H (*(vs32 *)(0x040002AC))

#define REG_SQRTCNT         (*(vu16 *)(0x040002B0))
#define REG_SQRT_PARAM      (*(vu64 *)(0x040002B8))
#define REG_SQRT_PARAM_L    (*(vu32 *)(0x040002B8))
#define REG_SQRT_PARAM_H    (*(vu32 *)(0x040002BC))
#define REG_SQRT_RESULT     (*(vu32 *)(0x040002B4))

// Math coprocessor modes

#define DIV_64_64           2
#define DIV_64_32           1
#define DIV_32_32           0
#define DIV_BUSY            (1u << 15)
#define DIV_MODE_MASK       3

#define SQRT_64             1
#define SQRT_32             0
#define SQRT_BUSY           (1u << 15)
#define SQRT_MODE_MASK      1

// Fixed point conversion macros

#define inttof32(n)         ((n) * (1 << 12)) ///< Convert int to f32
#define f32toint(n)         ((n) / (1 << 12)) ///< Convert f32 to int
#define floattof32(n)       ((int)((n) * (1 << 12))) ///< Convert float to f32
#define f32tofloat(n)       (((float)(n)) / (float)(1 << 12)) ///< Convert f32 to float

// Fixed Point versions

/// Asynchronous fixed point divide start
///
/// @param num
///     20.12 numerator.
/// @param den
///     20.12 denominator.
static inline void divf32_asynch(int32_t num, int32_t den)
{
    REG_DIV_NUMER = (int64_t)((uint64_t)(int64_t)num << 12);
    REG_DIV_DENOM_L = den;

    if ((REG_DIVCNT & DIV_MODE_MASK) != DIV_64_32)
        REG_DIVCNT = DIV_64_32;
}

/// Asynchronous fixed point divide result
///
/// @return
///     Returns 20.12 result.
static inline int32_t divf32_result(void)
{
    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIV_RESULT_L;
}

/// Fixed point divide
///
/// @param num
///     20.12 numerator.
/// @param den
///     20.12 denominator.
///
/// @return
///     Returns 20.12 result.
static inline int32_t divf32(int32_t num, int32_t den)
{
    divf32_asynch(num, den);
    return divf32_result();
}

/// Fixed point multiply.
///
/// @param a
///     20.12 value.
/// @param b
///     20.12 value.
///
/// @return
///     Returns 20.12 result.
static inline int32_t mulf32(int32_t a, int32_t b)
{
    int64_t result = (int64_t)a * (int64_t)b;
    return (int32_t)(result >> 12);
}

/// Asynchronous fixed point sqrt start.
///
/// @param a
///     20.12 positive value.
static inline void sqrtf32_asynch(uint32_t a)
{
    REG_SQRT_PARAM = ((uint64_t)a) << 12;

    if ((REG_SQRTCNT & SQRT_MODE_MASK) != SQRT_64)
        REG_SQRTCNT = SQRT_64;
}

/// Asynchronous fixed point sqrt result.
///
/// @return
///     20.12 result.
static inline uint32_t sqrtf32_result(void)
{
    while (REG_SQRTCNT & SQRT_BUSY);

    return REG_SQRT_RESULT;
}

#pragma GCC diagnostic push

// clang does not recognize -Wbuiltin-declaration-mismatch, ignore it
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
#endif

/// Fixed point sqrt.
///
/// @param a
///     20.12 positive value.
///
/// @return
///     20.12 result.
static inline uint32_t sqrtf32(uint32_t a)
{
    sqrtf32_asynch(a);
    return sqrtf32_result();
}
// restore previous diagnostic settings (works with GCC and clang)
#pragma GCC diagnostic pop

/// Asynchronous integer divide start.
///
/// @param num
///     Numerator.
/// @param den
///     Denominator.
static inline void div32_asynch(int32_t num, int32_t den)
{
    REG_DIV_NUMER_L = num;
    REG_DIV_DENOM_L = den;

    if ((REG_DIVCNT & DIV_MODE_MASK) != DIV_32_32)
        REG_DIVCNT = DIV_32_32;
}

/// Asynchronous integer divide result.
///
/// @return
///     32 bit integer result.
static inline int32_t div32_result(void)
{
    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIV_RESULT_L;
}

/// Integer divide.
///
/// @param num
///     Numerator.
/// @param den
///     Denominator.
///
/// @return
///     32 bit integer result.
static inline int32_t div32(int32_t num, int32_t den)
{
    div32_asynch(num, den);
    return div32_result();
}

/// Asynchronous integer modulo start.
///
/// @param num
///     Numerator.
/// @param den
///     Denominator.
static inline void mod32_asynch(int32_t num, int32_t den)
{
    REG_DIV_NUMER_L = num;
    REG_DIV_DENOM_L = den;

    if ((REG_DIVCNT & DIV_MODE_MASK) != DIV_32_32)
        REG_DIVCNT = DIV_32_32;
}

/// Asynchronous integer modulo result.
///
/// @return
///     32 bit integer remainder.
static inline int32_t mod32_result(void)
{
    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIVREM_RESULT_L;
}

/// Integer modulo.
///
/// @param num
///     Numerator.
/// @param den
///     Denominator.
///
/// @return
///     32 bit integer remainder.
static inline int32_t mod32(int32_t num, int32_t den)
{
    mod32_asynch(num, den);
    return mod32_result();
}

/// Asynchronous integer 64 bit divide start.
///
/// @param num
///     64 bit numerator.
/// @param den
///     32 bit denominator.
static inline void div64_asynch(int64_t num, int32_t den)
{
    REG_DIV_NUMER = num;
    REG_DIV_DENOM_L = den;

    if ((REG_DIVCNT & DIV_MODE_MASK) != DIV_64_32)
        REG_DIVCNT = DIV_64_32;
}

/// Asynchronous integer 64 bit divide result.
///
/// @return
///     32 bit integer result.
static inline int32_t div64_result(void)
{
    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIV_RESULT_L;
}

/// Integer 64 bit divide.
///
/// @param num
///     64 bit numerator.
/// @param den
///     32 bit denominator.
///
/// @return
///     32 bit integer result.
static inline int32_t div64(int64_t num, int32_t den)
{
    div64_asynch(num, den);
    return div64_result();
}

/// Asynchronous integer 64 bit modulo start.
///
/// @param num
///     64 bit numerator.
/// @param den
///     32 bit denominator.
static inline void mod64_asynch(int64_t num, int32_t den)
{
    REG_DIV_NUMER = num;
    REG_DIV_DENOM_L = den;

    if ((REG_DIVCNT & DIV_MODE_MASK) != DIV_64_32)
        REG_DIVCNT = DIV_64_32;
}

/// Asynchronous integer 64 bit modulo result.
///
/// @return
///     Returns 32 bit integer remainder.
static inline int32_t mod64_result(void)
{
    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIVREM_RESULT_L;
}

/// Integer 64 bit modulo.
///
/// @param num
///     64 bit numerator.
/// @param den
///     32 bit denominator.
///
/// @return
///     Returns 32 bit integer remainder.
static inline int32_t mod64(int64_t num, int32_t den)
{
    mod64_asynch(num, den);
    return mod64_result();
}

/// Asynchronous 32-bit integer sqrt start.
///
/// @param a
///     32 bit positive integer value.
static inline void sqrt32_asynch(uint32_t a)
{
    REG_SQRT_PARAM_L = a;

    if ((REG_SQRTCNT & SQRT_MODE_MASK) != SQRT_32)
        REG_SQRTCNT = SQRT_32;
}

/// Asynchronous 32-bit integer sqrt result.
///
/// @return
///     32 bit integer result.
static inline uint32_t sqrt32_result(void)
{
    while (REG_SQRTCNT & SQRT_BUSY);

    return REG_SQRT_RESULT;
}

/// 32-bit integer sqrt.
///
/// @param a
///     32 bit positive integer value.
///
/// @return
///     32 bit integer result.
static inline uint32_t sqrt32(uint32_t a)
{
    sqrt32_asynch(a);
    return sqrt32_result();
}

/// Asynchronous 64-bit integer sqrt start.
///
/// @param a
///     64 bit positive integer value.
static inline void sqrt64_asynch(uint64_t a)
{
    REG_SQRT_PARAM = a;

    if ((REG_SQRTCNT & SQRT_MODE_MASK) != SQRT_64)
        REG_SQRTCNT = SQRT_64;
}

/// Asynchronous 64-bit integer sqrt result.
///
/// @return
///     32 bit integer result.
static inline uint32_t sqrt64_result(void)
{
    while (REG_SQRTCNT & SQRT_BUSY);

    return REG_SQRT_RESULT;
}

/// 64-bit integer sqrt.
///
/// @param a
///     64 bit positive integer value.
///
/// @return
///     32 bit integer result.
static inline uint32_t sqrt64(uint64_t a)
{
    sqrt64_asynch(a);
    return sqrt64_result();
}

/// 32-bit floating point sqrt
///
/// @warning
///     Not safe to call inside an interrupt handler.
///
/// @param x
///     Valid 32 bit non-negative floating point value.
///
/// @return
///     32 bit floating point value
ARM_CODE float hw_sqrtf(float x);

/// 20.12 fixed point cross product.
///
/// Cross product:
///
///     result = A x B
///
///     x = Ay * Bz - By * Az
///     y = Az * Bx - Bz * Ax
///     z = Ax * By - Bx * Ay
///
/// @param a
///     Pointer to fixed 3 dimensions vector.
/// @param b
///     Pointer to fixed 3 dimensions vector.
/// @param result
///     Result pointer to fixed 3x3 matrix
static inline void crossf32(int32_t *a, int32_t *b, int32_t *result)
{
    result[0] = mulf32(a[1], b[2]) - mulf32(b[1], a[2]);
    result[1] = mulf32(a[2], b[0]) - mulf32(b[2], a[0]);
    result[2] = mulf32(a[0], b[1]) - mulf32(b[0], a[1]);
}

/// 20.12 fixed point dot product.
///
/// Dot Product:
///
///     Result = Ax * Bx + Ay * By + Az * Bz
///
/// @param a
///     Pointer to fixed 3 dimensions vector.
/// @param b
///     Pointer to fixed 3 dimensions vector.
///
/// @return
///     32 bit integer result
static inline int32_t dotf32(int32_t *a, int32_t *b)
{
    return mulf32(a[0], b[0]) + mulf32(a[1], b[1]) + mulf32(a[2], b[2]);
}

/// 20.12 fixed point normalize (set magnitude to 1.0 and keep the direction).
///
/// @param a
///     Pointer to the vector to normalize.
void normalizef32(int32_t *a);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_MATH_H__
