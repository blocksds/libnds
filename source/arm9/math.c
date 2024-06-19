// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Dominik Kurz

#include <nds/arm9/math.h>

#define QUIET_NAN ((255 << 23) | ((1 << 22) | 1))
#define INF (0xFF << 23)

ARM_CODE float hw_sqrtf(float x)
{
    union
    {
        float f;
        u32 i;
    } xu;

    xu.f = x;
    u32 mantissa32 = xu.i & ((1 << 23) - 1); // extracts mantissa via bitmask
    mantissa32 += 1 << 23; // adds implicit bit to mantissa.
    // If you were planning to handle subnormal numbers
    // you would check if the number is subnormal before
    // adding the implicit bit.
    // You would also need to do the shift of the mantissa and exponent
    // differently, probably should use __builtin_clz for this
    // as the count leading zeroes (clz) instruction is 1 cycle on arm
    // but this is not my problem
    // performance-critical code has subnormals turned off
    // due to -ffast-math
    u64 mantissa = mantissa32; // cast so that the shift doesnt go wrong
    mantissa <<= ((xu.i & (1 << 23)) == 0) + 25;
    // applies additional shift depending on whether exponent is even or odd
    // 25 is 23+2, the +2 is necessary for rounding
    REG_SQRT_PARAM = mantissa;
    if ((REG_SQRTCNT & SQRT_MODE_MASK) != SQRT_64)
    {
        REG_SQRTCNT = SQRT_64;
    }
    // starts hardware squareroot
    // It is critical that this happens as early as possible so that
    // we have time to do other stuff in the meantime
    u32 raw_exponent = xu.i & (255 << 23);
    u32 sign = xu.i & (1 << 31);
    // check if exponent is 0
    if (raw_exponent != 0)
    {
        if (sign || (xu.i > (255 << 23))) //check if negative or NaN
        {
            // Expected behavior:
            // sqrt(-f)=+qNaN, sqrt(-NaN)=+qNaN,sqrt(-Inf)=+qNaN
            // The IEEE 754 standard doesnt specify how signalling NaN
            // is encoded. Hard float arm stuff does it with a 1 in the least
            // significant bit and zeros in the rest of the mantissa
            // Quiet NaNs  on ARM are a 1 in
            // the most and least significant bit of the mantissa.
            // Example:
            // Quiet NaN mantissa (100000.....00001)
            // Signalling NaN mantissa (00000.....00001)
            xu.i = QUIET_NAN;
            return xu.f;
        }
        else if (raw_exponent == (255  << 23)) // check for +Inf
        {
            xu.i = INF;
            return xu.f; // return Inf
        }
        s32 exponent = raw_exponent - (127 << 23);
        // subtract bias from exponent
        exponent >>= 1;
        // This is meant to be a floor division
        // meaning -1/2= -0.5 should map to -1
        // if your compiler does logical shifts instead of arithmetic
        // use a division by 2.
        // arithmetic shift and division have different rounding behavior
        // but in this case it doesnt matter
        exponent = (exponent + (127 << 23)) & (0xff << 23);
        // add bias again and bitmask the exponent
        while (REG_SQRTCNT & SQRT_BUSY);
        // waits for square root operation to complete
        u32 new_mantissa = REG_SQRT_RESULT;
        new_mantissa += 1; // necessary for rounding
        new_mantissa >>= 1;// shift down after rounding
        // construct float from exponent and mantissa
        xu.i = exponent | (new_mantissa & ((1 << 23) - 1));
        return xu.f;
    }
    else
    {
        // numbers in this branch are either 0 or subnormal
        // if someone wants to add subnormal number handling
        // this would be the branch to do it
        // right now it assumes subnormal numbers are 0.
        // the --fast-math option causes other functions to generate 0
        // if they would otherwise generate a subnormal number
        // so maybe make sure subnormal number handling is done with a define
        // that checks if this option is present
        xu.i = sign;
        return xu.f; // returns +0 or -0 as appropriate
    }
}
