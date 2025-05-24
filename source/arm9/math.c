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

__attribute__((noinline)) ARM_CODE void normalizef32(int32_t *a)
{
    register int32_t *r0 asm("r0")=a;
    register int32_t r1 asm("r1");
    register int32_t r2 asm("r2");
    register int32_t r3 asm("r3");
asm (
    ".syntax unified; \n\t"
    "ldm r0, {r1,r2,r3} ; \n\t"
    :[r1]"=r"(r1), [r2]"=r"(r2), [r3]"=r"(r3): [r0]"r"(r0):
    );
    uint64_t msquared = (uint64_t)((int64_t)r1*r1) + (uint64_t)((int64_t)r2* r2) + (uint64_t)((int64_t)r3* r3);
    if (unlikely(msquared == 0))
        return;
    int clz =33-__builtin_clzll(msquared);//33=32+(1sign bit)
    clz +=1;//force shift to be even
    clz >>=1;
    clz <<=1;
    msquared= clz >= 0 ? msquared >> clz : msquared << -clz;
    REG_DIV_NUMER = 1ull << 62;
    REG_DIV_DENOM_L = (int32_t)msquared;
    if ((REG_DIVCNT & DIV_MODE_MASK) != DIV_64_32)
        REG_DIVCNT = DIV_64_32;
    sqrt64_asynch(msquared << 32);
    int total_shift=(32+8-6+(clz >> 1));
    uint32_t root=sqrt64_result();
    while (REG_DIVCNT & DIV_BUSY);

    int64_t reciprocal64=REG_DIV_RESULT; //64-bit result is required
    int shift=32-__builtin_clz((uint32_t)(reciprocal64>>32));
    total_shift-=shift;
    reciprocal64=reciprocal64>>shift;
    uint32_t reciprocal=(uint32_t)reciprocal64;
    uint32_t mulHi=((uint64_t)reciprocal*root) >> 32;
    #ifndef __OPTIMIZE_SIZE__
    #pragma GCC unroll 3
    #endif
    for (int i=0; i<3;i++)
    {
        int32_t t=a[i];
        int32_t st=t;
        if (t<0)
            st=-st;
        int32_t prod=((uint64_t)(uint32_t)st*mulHi)>>(total_shift);
        if (t<0)
            prod=-prod;
        a[i]=prod;
    }
    return;
}
