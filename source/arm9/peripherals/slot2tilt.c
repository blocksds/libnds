// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

#include <stdbool.h>
#include <stdint.h>
#include <nds/ndstypes.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/arm9/peripherals/slot2.h>
#include <nds/arm9/peripherals/slot2tilt.h>

#define TILT_SAMPLE1 (*(vuint8 *)0x0A008000)
#define TILT_SAMPLE2 (*(vuint8 *)0x0A008100)
#define TILT_X_LOW   (*(vuint8 *)0x0A008200)
#define TILT_X_HIGH  (*(vuint8 *)0x0A008300)
#define TILT_Y_LOW   (*(vuint8 *)0x0A008400)
#define TILT_Y_HIGH  (*(vuint8 *)0x0A008500)

bool peripheralSlot2TiltStart(void) {
    if (!peripheralSlot2Open(SLOT2_PERIPHERAL_TILT))
        return false;

    TILT_SAMPLE1 = 0x55;
    TILT_SAMPLE2 = 0xAA;
    return true;
}

bool peripheralSlot2TiltRead(slot2TiltPosition *data) {
    if (!peripheralSlot2Open(SLOT2_PERIPHERAL_TILT))
        return false;

    if (!(TILT_X_HIGH & 0x80))
        return false;

    if (data)
    {
        data->x = TILT_X_LOW | ((TILT_X_HIGH & 0x0F) << 8);
        data->y = TILT_Y_LOW | ((TILT_Y_HIGH & 0x0F) << 8);
    }
    return true;
}
