// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005-2010 Michael Noland (joat)
// Copyright (C) 2005-2010 Dave Murphy (WinterMute)

#include <nds/arm7/serial.h>
#include <nds/interrupts.h>
#include <nds/system.h>

int writePowerManagement(int reg, int command)
{
    int oldIME = enterCriticalSection();

    // Write the register / access mode (bit 7 sets access mode)
    while (REG_SPICNT & SPI_BUSY);

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_1MHz | SPI_BYTE_MODE | SPI_CONTINUOUS
               | SPI_DEVICE_POWER;
    REG_SPIDATA = reg;

    // Write the command / start a read
    while (REG_SPICNT & SPI_BUSY);

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_1MHz | SPI_BYTE_MODE | SPI_DEVICE_POWER;
    REG_SPIDATA = command;

    // Read the result
    while (REG_SPICNT & SPI_BUSY);

    leaveCriticalSection(oldIME);

    return REG_SPIDATA & 0xFF;
}

void ledBlink(int value)
{
    u32 temp = readPowerManagement(PM_CONTROL_REG);
    temp &= ~PM_LED_CONTROL_MASK;
    temp |= PM_LED_CONTROL(value & 3);
    writePowerManagement(PM_CONTROL_REG, temp);
}
