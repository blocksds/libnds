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

uint16_t peripheralSlot2TiltGetX(void) {
    return TILT_X_LOW | ((TILT_X_HIGH & 0x0F) << 8);
}

uint16_t peripheralSlot2TiltGetY(void) {
    return TILT_Y_LOW | ((TILT_Y_HIGH & 0x0F) << 8);
}

bool peripheralSlot2TiltUpdate(void) {
    if (!peripheralSlot2Open(SLOT2_PERIPHERAL_TILT))
        return false;

    if (!(TILT_X_HIGH & 0x80))
        return false;

    TILT_SAMPLE1 = 0x55;
    TILT_SAMPLE2 = 0xAA;
    return true;

}
