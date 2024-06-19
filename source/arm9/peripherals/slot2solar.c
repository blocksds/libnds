// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#include <nds/ndstypes.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/arm9/peripherals/slot2.h>
#include <nds/arm9/peripherals/slot2solar.h>

#define GPIO_DATA (*(vuint16 *)0x080000C4)

int peripheralSlot2SolarScanFast(void)
{
    if (!peripheralSlot2Open(SLOT2_PERIPHERAL_SOLAR_GPIO))
        return -1;

    int oldIME = enterCriticalSection();

    // reset binary counter
    GPIO_DATA = 0x02;
    GPIO_DATA = 0x00;

    // count
    int result = 0;
    do
    {
        // increment binary counter
        GPIO_DATA = 0x01;
        GPIO_DATA = 0x00;
    } while (!(GPIO_DATA & 0x08) && result < 0x100);

    leaveCriticalSection(oldIME);

    return result < 0x100 ? result : -1;
}
