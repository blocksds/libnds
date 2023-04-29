// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file math.h
///
/// @brief hardware coprocessor math instructions.

#ifndef LIBNDS_NDS_ARM9_MATH_H__
#define LIBNDS_NDS_ARM9_MATH_H__

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
#define REG_SQRT_PARAM      (*(vs64 *)(0x040002B8))
#define REG_SQRT_PARAM_L    (*(vs32 *)(0x040002B8))
#define REG_SQRT_PARAM_H    (*(vs32 *)(0x040002BC))
#define REG_SQRT_RESULT     (*(vu32 *)(0x040002B4))

// Math coprocessor modes

#define DIV_64_64           2
#define DIV_64_32           1
#define DIV_32_32           0
#define DIV_BUSY            (1 << 15)

#define SQRT_64             1
#define SQRT_32             0
#define SQRT_BUSY           (1 << 15)

// Fixed point conversion macros

#define inttof32(n)         ((n) * (1 << 12)) ///< Convert int to f32
#define f32toint(n)         ((n) / (1 << 12)) ///< Convert f32 to int
#define floattof32(n)       ((int)((n) * (1 << 12))) ///< Convert float to f32
#define f32tofloat(n)       (((float)(n)) / (float)(1 << 12)) ///< Convert f32 to float

// Fixed Point versions

/// Fixed point divide
///
/// @param num 20.12 numerator.
/// @param den 20.12 denominator.
/// @return returns 20.12 result.
static inline int32_t divf32(int32_t num, int32_t den)
{
    REG_DIVCNT = DIV_64_32;

    while (REG_DIVCNT & DIV_BUSY);

    REG_DIV_NUMER = ((int64_t)num) << 12;
    REG_DIV_DENOM_L = den;

    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIV_RESULT_L;
}

/// Fixed point multiply.
///
/// @param a 20.12 value.
/// @param b 20.12 value.
/// @return returns 20.12 result.
static inline int32_t mulf32(int32_t a, int32_t b)
{
    int64_t result = (int64_t)a * (int64_t)b;
    return (int32_t)(result >> 12);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbuiltin-declaration-mismatch"
/// Fixed point sqrt.
///
/// @param a 20.12 value.
/// @return 20.12 result.
static inline int32_t sqrtf32(int32_t a)
{
    REG_SQRTCNT = SQRT_64;

    while (REG_SQRTCNT & SQRT_BUSY);

    REG_SQRT_PARAM = ((int64_t)a) << 12;

    while (REG_SQRTCNT & SQRT_BUSY);

    return REG_SQRT_RESULT;
}
#pragma GCC diagnostic pop

// Integer versions

/// Integer divide.
///
/// @param num Numerator.
/// @param den Denominator.
/// @return 32 bit integer result.
static inline int32_t div32(int32_t num, int32_t den)
{
    REG_DIVCNT = DIV_32_32;

    while (REG_DIVCNT & DIV_BUSY);

    REG_DIV_NUMER_L = num;
    REG_DIV_DENOM_L = den;

    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIV_RESULT_L;
}

/// Integer modulo.
///
/// @param num Numerator.
/// @param den Denominator.
/// @return 32 bit integer remainder.
static inline int32_t mod32(int32_t num, int32_t den)
{
    REG_DIVCNT = DIV_32_32;

    while (REG_DIVCNT & DIV_BUSY);

    REG_DIV_NUMER_L = num;
    REG_DIV_DENOM_L = den;

    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIVREM_RESULT_L;
}

/// Integer 64 bit divide.
///
/// @param num 64 bit numerator.
/// @param den 32 bit denominator.
/// @return 32 bit integer result.
static inline int32_t div64(int64_t num, int32_t den)
{
    REG_DIVCNT = DIV_64_32;

    while (REG_DIVCNT & DIV_BUSY);

    REG_DIV_NUMER = num;
    REG_DIV_DENOM_L = den;

    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIV_RESULT_L;
}

/// Integer 64 bit modulo.
///
/// @param num 64 bit numerator.
/// @param den 32 bit denominator.
/// @return returns 32 bit integer remainder.
static inline int32_t mod64(int64_t num, int32_t den)
{
    REG_DIVCNT = DIV_64_32;

    while (REG_DIVCNT & DIV_BUSY);

    REG_DIV_NUMER = num;
    REG_DIV_DENOM_L = den;

    while (REG_DIVCNT & DIV_BUSY);

    return REG_DIVREM_RESULT_L;
}

/// Integer sqrt.
///
/// @param a 32 bit integer value.
/// @return 32 bit integer result.
static inline u32 sqrt32(int a)
{
    REG_SQRTCNT = SQRT_32;

    while (REG_SQRTCNT & SQRT_BUSY);

    REG_SQRT_PARAM_L = a;

    while (REG_SQRTCNT & SQRT_BUSY);

    return REG_SQRT_RESULT;
}

/// Integer sqrt.
///
/// @param a 64 bit integer value.
/// @return 32 bit integer result.
static inline u32 sqrt64(long long a)
{
    REG_SQRTCNT = SQRT_64;

    while (REG_SQRTCNT & SQRT_BUSY);

    REG_SQRT_PARAM = a;

    while (REG_SQRTCNT & SQRT_BUSY);

    return REG_SQRT_RESULT;
}

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
/// @param a Pointer to fixed 3 dimensions vector.
/// @param b Pointer to fixed 3 dimensions vector.
/// @param result Result pointer to fixed 3x3 matrix
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
/// @param a Pointer to fixed 3 dimensions vector.
/// @param b Pointer to fixed 3 dimensions vector.
/// @return 32 bit integer result
static inline int32_t dotf32(int32_t *a, int32_t *b)
{
    return mulf32(a[0], b[0]) + mulf32(a[1], b[1]) + mulf32(a[2], b[2]);
}

/// 20.12 fixed point normalize (set magnitude to 1.0 and keep the direction).
///
/// @param a Pointer to the vector to normalize.
static inline void normalizef32(int32_t *a)
{
    // magnitude = sqrt(Ax^2 + Ay^2 + Az^2)
    int32_t magnitude = sqrtf32(mulf32(a[0], a[0]) + mulf32(a[1], a[1]) + mulf32(a[2], a[2]));

    a[0] = divf32(a[0], magnitude);
    a[1] = divf32(a[1], magnitude);
    a[2] = divf32(a[2], magnitude);
}

#endif // LIBNDS_NDS_ARM9_MATH_H__
