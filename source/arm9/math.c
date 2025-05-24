// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Dominik Kurz

#include <stdint.h>

#include <nds/arm9/math.h>

#define QUIET_NAN       ((255 << 23) | ((1 << 22) | 1))
#define INF             (0xFF << 23)

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect((x), 0)

ARM_CODE float hw_sqrtf(float x)
{
    union
    {
        float f;
        uint32_t i;
    } xu;

    xu.f = x;
    int32_t exponent = (int32_t)xu.i >> 23;

    // check if exponent is 0
    if (likely(exponent > 0))
    {
        if (unlikely(exponent == 255)) // Check if negative or NaN
        {
            // Expected behavior:
            // sqrt(-f) = +qNaN, sqrt(-NaN) = +qNaN, sqrt(-Inf) = +qNaN
            xu.i = (xu.i == INF) ? INF : QUIET_NAN;
            return xu.f;
        }
        else
        {
            uint32_t mantissa = xu.i & ~((uint32_t)exponent << 23);
            exponent = exponent - 127;
            mantissa += 1 << 23; // Adds implicit bit to mantissa.
            mantissa <<= (exponent & 1);

            REG_SQRT_PARAM = ((uint64_t)mantissa) << 25;
            if ((REG_SQRTCNT & SQRT_MODE_MASK) != SQRT_64)
                REG_SQRTCNT = SQRT_64;

            exponent >>= 1;
            // This is meant to be a floor division
            // meaning -1/2= -0.5 should map to -1
            exponent = exponent + 126;
            exponent <<= 23;
            // Wait for the square root operation to complete
            while (REG_SQRTCNT & SQRT_BUSY);

            uint32_t new_mantissa = REG_SQRT_RESULT;
            new_mantissa += 1;
            xu.i = ((uint32_t)exponent) + (new_mantissa >> 1);
            return xu.f;
        }
    }
    else
    {
        if (likely(exponent == 0))
        {
            if (likely(xu.i != 0))
            {
                uint32_t mantissa = xu.i & ~((uint32_t)exponent << 23);
                int32_t shift = __builtin_clz(mantissa) - (31 - 23);
                mantissa <<= shift; // normalize subnormal

                exponent = -126 - shift;
                mantissa <<= (exponent & 1);

                REG_SQRT_PARAM = ((uint64_t)mantissa) << 25;
                if ((REG_SQRTCNT & SQRT_MODE_MASK) != SQRT_64)
                    REG_SQRTCNT = SQRT_64;

                exponent >>= 1;
                exponent = exponent + 126;
                exponent <<= 23;

                // Wait for the square root operation to complete
                while (REG_SQRTCNT & SQRT_BUSY);

                uint32_t new_mantissa = REG_SQRT_RESULT;
                new_mantissa += 1;
                xu.i = ((uint32_t)exponent) + (new_mantissa >> 1);
                return xu.f;
            }
            else // sqrt(+0) = +0
            {
                xu.i = 0;
                return xu.f;
            }
        }
        else if (xu.i == (1u << 31))
        {
            xu.i= 1u << 31; // sqrt(-0) = -0
            return xu.f;
        }
        else
        {
            xu.i=QUIET_NAN; // sqrt(negative) = qNaN
            return xu.f;
        }
    }
}
