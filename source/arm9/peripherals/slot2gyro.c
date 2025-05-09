// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#include <nds/ndstypes.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/arm9/peripherals/slot2.h>
#include <nds/arm9/peripherals/slot2gyro.h>

#define GPIO_DATA    (*(vuint16 *)0x080000C4)
#define GPIO_CONTROL (*(vuint16 *)0x080000C8)
#define GPIO_SENSE_START  0x01
#define GPIO_SERIAL_CLOCK 0x02

int peripheralSlot2GyroScan(void)
{
    if (!peripheralSlot2Open(SLOT2_PERIPHERAL_GYRO_GPIO))
        return -1;

    uint16_t gpio = GPIO_DATA & 0x08;

    // start measurement
    GPIO_DATA = gpio | GPIO_SENSE_START | GPIO_SERIAL_CLOCK;
    GPIO_DATA = gpio | GPIO_SERIAL_CLOCK;
    GPIO_DATA = gpio;

    uint16_t result = 0;
    for (int i = 0; i < 16; i++)
    {
        result = (result << 1) | ((GPIO_DATA >> 2) & 1);
        GPIO_DATA = gpio;
        // introduce delay
        GPIO_CONTROL;
        GPIO_CONTROL;
        GPIO_CONTROL;
        GPIO_DATA = gpio | GPIO_SERIAL_CLOCK;
    }
    return result & 0xFFF;
}
