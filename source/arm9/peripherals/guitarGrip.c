// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 zeromus
// Copyright (C) 2011 Dave Murphy (WinterMute)

#include <nds/arm9/guitarGrip.h>
#include <nds/memory.h>
#include <nds/system.h>

static u8 guitar_keys = 0;
static u8 guitar_keys_old = 0;

static void guitarGripSetBus(void)
{
    // Setting the bus owner is not sufficient, as we need to ensure that the
    // bus speeds are adequately slowed. this magic number contains the
    // appropriate timings.

    REG_EXMEMCNT &= ~EXMEMCNT_CART_ARM7;
    REG_EXMEMCNT &= ~(EXMEMCNT_SRAM_TIME_MASK | EXMEMCNT_ROM_TIME1_MASK
                      | EXMEMCNT_ROM_TIME2_MASK | EXMEMCNT_PHI_CLOCK_MASK);
    REG_EXMEMCNT |= (EXMEMCNT_SRAM_TIME_10_CYCLES | EXMEMCNT_ROM_TIME1_18_CYCLES
                     | EXMEMCNT_ROM_TIME2_6_CYCLES | EXMEMCNT_PHI_CLOCK_OFF);
}

bool guitarGripIsInserted(void)
{
    // Accessing the slot-2 memory region in DSi mode will cause a MPU
    // exception, so this code can't run on a DSi at all.
    if (isDSiMode())
        return false;

    guitarGripSetBus();

    // This is 0x96h is a GBA game is inserted
    if (GBA_HEADER.is96h == 0x96)
        return false;

    // Guitar grip signifies itself this way
    if (*(vu16 *)0x08000000 != 0xF9FF)
        return false;

    return true;
}

void guitarGripScanKeys(void)
{
    guitarGripSetBus();
    guitar_keys_old = guitar_keys;
    guitar_keys = ~(*(vu8 *)0x0A000000);
}

u8 guitarGripKeysHeld(void)
{
    return guitar_keys;
}

u16 guitarGripKeysDown(void)
{
    return guitar_keys & ~guitar_keys_old;
}

u16 guitarGripKeysUp(void)
{
    return (guitar_keys ^ guitar_keys_old) & ~guitar_keys;
}
