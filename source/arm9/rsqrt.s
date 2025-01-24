// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 aikku93


#include <nds/asminc.h>
// 2^N+1 entries in the LUT
#define RECPLUT_LENGTH_BITS 5
#define QUIET_NAN ((255 << 23) | ((1 << 22) | 1))
#define INF (0xFF << 23)
#ifndef ARM9
#error "ARM9 must be defined."
#endif
.syntax  unified
.arch    armv5te
.cpu     arm946e-s
.arm
BEGIN_ASM_FUNC hw_rsqrtf_asm
    asrs    r1, r0, #23                 @ Exponent -> r1, and check for negative and subnormals
    bmi     .LinputIsNegative
    beq     .LinputIsSubnormal
    bic     r0, r0, r1, lsl #23         @ Significand -> r0
    add     r0, #1 << 23

    tst     r1, #1                      @ Test even/odd exponent
    mov     r2, #0
    lslne   r3, r0, #7                  @ Even exponent: Keep significand as-is
    lsleq   r3, r0, #6                  @ Odd exponent:  Shift significand down one bit
    mov     ip, #0x04000001             @ &REG_SQRTCNT + 01h -> ip
    add     ip, #0x000002B0
    strd    r2, [ip, #8-1]              @ Start square root of significand
    ldr     r2, [ip, #0-1]              @ Ensure we use 64-bit mode
    tst     r2, #1
    streq   ip, [ip, #0-1]
    cmp     r1, #255                    @ Check for Inf/NaN inputs
    beq     .LinputIsInfOrNaN

    ldr     ip, =hw_rsqrtf_recpLUT - 4*(1<<RECPLUT_LENGTH_BITS)
    lsr     r2, r0, #23 - RECPLUT_LENGTH_BITS
    bic     r3, r0, r2, lsl #23 - RECPLUT_LENGTH_BITS
    add     ip, ip, r2, lsl #2          @ Build reciprocal approximation
    ldmia   ip, {r2,ip}
    str     lr, [sp, #-8]!              @ <- Awkwardly sandwiched to avoid a stall cycle
    rsb     ip, r2
    umull   r3, lr, ip, r3
    sub     r1, #127                    @ Remove exponent bias
    sub     r2, r2, r3, lsr #23 - RECPLUT_LENGTH_BITS
    sub     r2, r2, lr, lsl #32 - (23 - RECPLUT_LENGTH_BITS)

    umull   r3, ip, r2, r2              @ Perform Newton iteration
    asr     r1, #1                      @ Exponent /= 2
    umull   r3, lr, ip, r0
    rsb     r1, #127 - 1 - 1            @ Restore exponent bias and apply correction
    sub     r0, r2, r3, lsr #23
    sub     r0, r0, lr, lsl #32 - 23
    mov     ip, #0x04000000             @ Wait until square root is ready, then sqrtX - > r2
1:  ldr     r2, [ip, #0x2B0]
    tst     r2, #1 << 15
    bne     1b
    ldr     r2, [ip, #0x2B4]

    umull   r2, r3, r0, r2              @ (sqrtX * inv)|Exponent as result
    lsrs    r3, #5
    adc     r0, r3, r1, lsl #23
    ldr     pc, [sp], #8

.LinputIsNegative:
    ldr     r0, =QUIET_NAN              @ 1 / sqrt(-x) = qNaN
    bx      lr

.LinputIsSubnormal:
    ldr     r0, =INF                    @ 1 / sqrt(+0) = +Inf
    bx      lr

.LinputIsInfOrNaN:
    subs    r0, r0, r1, lsl #23         @ 1 / sqrt(+Inf) = 0
    ldrne   r0, =QUIET_NAN              @ 1 / sqrt(NaN) = NaN
    bx      lr

//.section .dtcm.hw_rsqrtf_recpLUT, "aw", %progbits
.section .rodata.hw_rsqrtf_recpLUT
.balign 4

hw_rsqrtf_recpLUT:
    .word 0x80000000,0x7C1F07C2,0x78787879,0x75075076,0x71C71C72,0x6EB3E454,0x6BCA1AF3,0x6906906A
    .word 0x66666667,0x63E7063F,0x61861862,0x5F417D06,0x5D1745D2,0x5B05B05C,0x590B2165,0x572620AF
    .word 0x55555556,0x5397829D,0x51EB851F,0x50505051,0x4EC4EC4F,0x4D4873ED,0x4BDA12F7,0x4A7904A8
    .word 0x4924924A,0x47DC11F8,0x469EE585,0x456C797E,0x44444445,0x4325C53F,0x42108422,0x41041042
    .word 0x40000000






/* //C-version for documentation purposes
#include <nds/arm9/math.h>

#define QUIET_NAN ((255 << 23) | ((1 << 22) | 1))
#define INF (0xFF << 23)

// 2^N+1 entries in the LUT
#define RECPLUT_LENGTH_BITS 5

// Mathematica formula:
//  Table[Ceiling[2^31/x], {x, 1, 2, 2^-RECPLUT_LENGTH_BITS}]
static const uint32_t recpLUT[32+1] = {
    0x80000000,0x7C1F07C2,0x78787879,0x75075076,0x71C71C72,0x6EB3E454,0x6BCA1AF3,0x6906906A,
    0x66666667,0x63E7063F,0x61861862,0x5F417D06,0x5D1745D2,0x5B05B05C,0x590B2165,0x572620AF,
    0x55555556,0x5397829D,0x51EB851F,0x50505051,0x4EC4EC4F,0x4D4873ED,0x4BDA12F7,0x4A7904A8,
    0x4924924A,0x47DC11F8,0x469EE585,0x456C797E,0x44444445,0x4325C53F,0x42108422,0x41041042,
    0x40000000
};

ARM_CODE float hw_rsqrtf(float x)
{
    union
    {
        float f;
        u32 i;
    } xu;
    xu.f = x;

    // Check exponent special cases
    // These really should be `asrs,bmi,beq`
    s32 exp = (s32)xu.i >> 23;
    if(exp < 0)
    {
        // 1 / sqrt(-x) = qNaN
        xu.i = QUIET_NAN;
        return xu.f;
    }
    else if(exp == 0)
    {
        // 1 / sqrt(+0) = +Inf
        // We could check for subnormals here if we wanted to
        xu.i = INF;
        return xu.f;
    }

    // Build the significand and start a square root as
    // soon as possible. We shift the 23-bit significand
    // up to 62-bit before the square root, which gives
    // us a 31-bit output. Also note for odd exponents,
    // we need a further shift up because we will drop
    // one bit from `exp`, and must account for it.
    u32 significand = xu.i &~ ((u32)exp << 23);
    significand += 1u << 23;
    {
        u64 sigBits = (u64)significand << (62-23);
        if((exp & 1) == 0) sigBits <<= 1;
        REG_SQRT_PARAM = sigBits;
    }
    if((REG_SQRTCNT & SQRT_MODE_MASK) != SQRT_64)
    {
        REG_SQRTCNT = SQRT_64;
    }

    // Check for Inf/NaN inputs
    if(exp == 255)
    {
        // 1 / sqrt(+Inf) = 0
        // 1 / sqrt(NaN) = NaN
        xu.i = (xu.i == INF) ? 0 : QUIET_NAN;
        return xu.f;
    }

    // First, get a rough estimate for `1/significand`.
    // `significand` is in range 2^23 .. 2^24-1, so we
    // split the calculation of `2^k/significand` into
    // `2^31/(x*2^-23)` to give us a 31-bit reciprocal.
    // The reciprocal is 31 bits wide, but is actually
    // 54 bits because of the scaling we just did.
    // `x*2^-23` gives us a value in range 1 inclusive
    // through 2 exclusive, so we use a lookup table
    // to get us most of the way to the result (using
    // the high bits of the significand), and we then
    // linearly interpolate the result to improve it
    // somewhat (this is actually more precise than
    // applying Newton's method, because we may start
    // much closer to the previous lookup entry than
    // to the next, which would slow down convergence).
    // Note that we can't use a 32bit reciprocal, as
    // when we move to the Newton iteration in the next
    // step, we could get a reciprocal of 2^32, which
    // then gets cast to 32bit to equal 0.
    u32 inv;
    {
        u32 sigInt  = (significand >> (23-RECPLUT_LENGTH_BITS));
        u32 sigFrac = significand &~ (sigInt << (23-RECPLUT_LENGTH_BITS));
        const u32 *lut = recpLUT - (1<<RECPLUT_LENGTH_BITS) + sigInt;
        u32 invA = lut[0];
        u32 invB = lut[1];

        // Note that we use `x - a*(x-y)` rather than the
        // usual `x + a*(y-x)`. This is because our entries
        // are monotonically /decreasing/, and we want an
        // unsigned multiply here.
        inv = invA - (u32)((u64)(invA - invB)*sigFrac >> (23-RECPLUT_LENGTH_BITS));
    }

    // Now apply Newton's method to improve the result,
    // but we only actually need a single iteration to
    // get us nearly perfect results.
    // The recurrence for 1/x is:
    //  y(n+1) = y(n)*(2 - x*y(n)) = 2*y(n) - x*y(n)*y(n)
    // However, in the interests of improved performance,
    // we actually /drop/ one bit here, which avoids us
    // having to shift the starting value on top of the
    // weighted difference. Note that the bit is being
    // dropped in the calculation of `inv*inv>>32` for
    // the sake of performance, but this doesn't affect
    // the precision of the output beyond 1ulp.
    inv = inv - (u32)(((u64)inv*inv >> 32) * significand >> 23);

    // If we had an odd exponent, we have to remember we
    // shifted the significand up by 1 bit, so account for
    // it now, by shifting the inverse /down/ 1 bit.
    if((exp & 1) == 0) inv >>= 1;

    // Because we calculated an inverse, our exponent needs
    // to be negated. Further, because we used a square root,
    // the exponent also needs to be halved, ensuring to use
    // a floor division by 2 since we already accounted for
    // odd exponent values. Finally, we need to subtract 1
    // from the exponent and shift the significand up one
    // bit to account for the bit dropped during sqrt.
    // As an optimization, we also subtract 1 from the new
    // exponent, because we can then use the fact that the
    // significand has the implicit bit already set at this
    // point, and we can then just add the fixed exponent.
    s32 newExp = -((exp - 127) >> 1) + 127 - 1 - 1;

    // Wait for our square root to be ready, as we are
    // bottlenecked by it rather than this code on DSi.
    while(REG_SQRTCNT & SQRT_BUSY);
    u32 sqrtX = REG_SQRT_RESULT;

    // Now that we have the inverse and the square root,
    // we can calculate rsqrt(x) using:
    //  rsqrt(x) = (x^0.5)^-1 = x^0.5 * x^-1
    // For the final shift down value, we have:
    //  -`sqrtX` is a Q31 value
    //  -`inv` is a Q53 value, and relative to a Q23 value
    //  -We want a Q23 output
    //  -The output significand needs to shift up 1 bit
    // Note that we also apply rounding here to improve the
    // precision of the results. We will be within 1ulp of
    // the target even without the rounding, but this will
    // improve exactnessness from <50% to >95%.
    const u32 shift = (31 + 53-23 - 23 - 1);
    const u64 bias = (1ull << shift) / 2;
    u32 newSignificand = (u32)(((u64)sqrtX * inv + bias) >> shift);

    // Finally, build the output float value
    xu.i = newSignificand + ((u32)newExp << 23);
    return xu.f;
} */
