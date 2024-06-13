// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka

#include <nds/arm7/tsc.h>
#include <nds/interrupts.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

uint16_t tscRead(uint32_t command)
{
    uint8_t msb, lsb;
    uint32_t oldIME = enterCriticalSection();

    spiWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC | SPI_CONTINUOUS;
    spiWrite(command);

    msb = spiRead() & 0x7F;

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC;
    lsb = spiRead() & 0xF8;

    leaveCriticalSection(oldIME);

    return (msb << 5) | (lsb >> 3);
}

// Perform a 16 clocks-per-conversion measurement.
void tscMeasure(uint32_t command, uint16_t *buffer, uint32_t count)
{
    if (!count) return;

    uint8_t msb, lsb;
    uint32_t oldIME = enterCriticalSection();

    spiWaitBusy();
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC | SPI_CONTINUOUS;

    spiWrite(command);

    // First .. second-to-last measurement
    while (--count)
    {
        msb = spiRead() & 0x7F;
        lsb = spiExchange(command) & 0xF8;

        *(buffer++) = (msb << 5) | (lsb >> 3);
    }

    // Last measurement
    msb = spiRead() & 0x7F;
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC;
    lsb = spiRead() & 0xF8;

    leaveCriticalSection(oldIME);

    *(buffer++) = (msb << 5) | (lsb >> 3);
}
