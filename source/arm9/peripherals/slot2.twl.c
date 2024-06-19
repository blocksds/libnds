// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <nds/ndstypes.h>
#include <nds/memory.h>
#include <nds/system.h>
#include <nds/arm9/peripherals/slot2.h>

// Points to ARM9 ROM size in the in-memory NDS header copy
#define TWL_RAM_TEST1 *((volatile uint32_t*) 0xCFFFE2C)
#define TWL_RAM_TEST2 *((volatile uint32_t*) 0xDFFFE2C)

bool slot2DetectTWLDebugRam(void)
{
    bool result = false;

    // Configure faux Slot-2 RAM
    REG_SCFG_EXT = REG_SCFG_EXT | SCFG_EXT_RAM_DEBUG | SCFG_EXT_RAM_TWL;
    // Temporarily change a safe-to-change value
    uint32_t old_test1 = TWL_RAM_TEST1;
    uint32_t old_test2 = TWL_RAM_TEST2;
    TWL_RAM_TEST1 = 0x0000;
    TWL_RAM_TEST2 = 0xFFFF;
    if (TWL_RAM_TEST1 == 0x0000 && TWL_RAM_TEST2 == 0xFFFF)
    {
        result = true;
    }
    TWL_RAM_TEST1 = old_test1;
    TWL_RAM_TEST2 = old_test2;
    return result;
}
