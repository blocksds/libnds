// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz
// Copyright (c) 2024 Adrian "asie" Siekierka

#ifndef ARM7_LIBNDS_INTERNAL_H__
#define ARM7_LIBNDS_INTERNAL_H__

#include <nds/ndstypes.h>

void storageMsgHandler(int bytes, void *user_data);
void storageValueHandler(u32 value, void *user_data);
void firmwareMsgHandler(int bytes, void *user_data);

typedef struct {
    u16 value;      // 1..4095, 0 if invalid
    u16 noisiness;  // 0..4095, ~15-16 = 1 pixel
} libnds_touchMeasurementFilterResult;

/**
 * @brief Perform filtering on the raw touch samples provided to return one
 * averaged sample and an estimate of how noisy it is, while skipping outliers.
 *
 * Internal. See touchFilter.c for more information.
 */
libnds_touchMeasurementFilterResult libnds_touchMeasurementFilter(u16 values[5]);

__attribute__((always_inline, noreturn))
THUMB_CODE static inline void libndsCrash(__attribute__((unused)) const char *message)
{
    // This function causes an undefined instruction exception to crash the CPU
    // in a controlled way.
    //
    // Opcodes defined as "Undefined Instruction" by the ARM architecture:
    //
    // - Thumb: 0xE800 | (ignored & 0x7FF)
    // - ARM:   0x?6000010 | (ignored & 0x1FFFFEF)
    //
    // It's better to use Thumb because that way the ARM7 can jump to it with a
    // BL instruction instead of BL+BX (BLX only exists in the ARM9)./
    asm volatile inline(".hword 0xE800 | 0xBAD");
    __builtin_unreachable();
}

#endif // ARM7_LIBNDS_INTERNAL_H__
