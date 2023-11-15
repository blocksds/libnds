// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 zeromus
// Copyright (C) 2011 Dave Murphy (WinterMute)

#include <nds/arm9/paddle.h>
#include <nds/memory.h>
#include <nds/system.h>

static void paddleSetBus(void)
{
    // Setting the bus owner is not sufficient, as we need to ensure that the
    // bus speeds are adequately slowed.

    REG_EXMEMCNT &= ~EXMEMCNT_CART_ARM7;
    REG_EXMEMCNT &= ~(EXMEMCNT_SRAM_TIME_MASK | EXMEMCNT_PHI_CLOCK_MASK);
    REG_EXMEMCNT |= (EXMEMCNT_SRAM_TIME_18_CYCLES | EXMEMCNT_PHI_CLOCK_4MHZ);
}

bool paddleIsInserted(void)
{
    // Accessing the slot-2 memory region in DSi mode will cause a MPU
    // exception, so this code can't run on a DSi at all.
    if (isDSiMode())
        return false;

    paddleSetBus();

    // This is 0x96h is a GBA game is inserted
    if (GBA_HEADER.is96h == 0x96)
        return false;

    // Paddle signifies itself this way
    if (*(vu16 *)0x08000000 != 0xEFFF)
        return false;

    // Let's check one more way just to be safe
    if (*(vu16 *)0x0A000002 != 0x0000)
        return false;

    return true;
}

u16 paddleRead(void)
{
    paddleSetBus();
    return (*(vu8 *)0x0A000000) | ((*(vu8 *)0x0A000001) << 8);
}

void paddleReset(void)
{
    (*(vu8 *)0x0A000000) = 0;
}
