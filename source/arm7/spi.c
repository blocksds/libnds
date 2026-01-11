// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2010 Michael Noland (joat)
// Copyright (C) 2005-2010 Dave Murphy (WinterMute)

#include <nds/arm7/serial.h>
#include <nds/interrupts.h>
#include <nds/system.h>

void spiWaitBusy(void)
{
    while (REG_SPICNT & SPI_BUSY);
}

u8 spiExchange(u8 value)
{
    REG_SPIDATA = value;
    spiWaitBusy();
    return REG_SPIDATA & 0xFF;
}

void spiWrite(u8 value)
{
    REG_SPIDATA = value;
    spiWaitBusy();
}

u8 spiRead(void)
{
    return spiExchange(0);
}

int writePowerManagement(int reg, int command)
{
    int oldIME = enterCriticalSection();
    spiWaitBusy();

    // Write the register / access mode (bit 7 sets access mode)
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_POWER | SPI_CONTINUOUS;
    spiWrite(reg);

    // Write the command / start a read
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_POWER;
    uint8_t value = spiExchange(command);

    REG_SPICNT = 0;
    leaveCriticalSection(oldIME);

    return value;
}

void ledBlink(PM_LedStates value)
{
    u32 temp = readPowerManagement(PM_CONTROL_REG);
    temp &= ~PM_LED_CONTROL_MASK;
    temp |= PM_LED_CONTROL(value & 3);
    writePowerManagement(PM_CONTROL_REG, temp);
}
