// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka

#include "nds/arm7/touch.h"
#include <nds/arm7/tsc.h>
#include <nds/interrupts.h>
#include <nds/ndstypes.h>
#include <nds/system.h>


// Internal. Read TSC data to buffer, excluding the first SPI write.
// No IRQ protection.
static void __tscReadToBuffer(u32 command, u16 *buffer, u32 count)
{
    u8 msb, lsb;

    // First .. second-to-last measurement
    while (--count)
    {
        msb = spiRead();
        lsb = spiExchange(command);

        *(buffer++) = ((msb << 5) | (lsb >> 3)) & 0xFFF;
    }

    // Last measurement
    msb = spiRead();
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC;
    lsb = spiRead();

    *(buffer++) = ((msb << 5) | (lsb >> 3)) & 0xFFF;
}

// Internal. Read five measurements from TSC, skipping the first.
// No IRQ protection.
static void __tscMeasureFiveSkipFirst(u32 command, u16 *buffer)
{
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC | SPI_CONTINUOUS;

    spiWrite(command);
    spiRead();
    spiWrite(command);

    __tscReadToBuffer(command, buffer, 5);
}

u16 tscRead(u32 command)
{
    uint8_t msb, lsb;
    uint32_t oldIME = enterCriticalSection();

    spiWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC | SPI_CONTINUOUS;
    spiWrite(command);

    msb = spiRead();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC;
    lsb = spiRead();

    leaveCriticalSection(oldIME);

    return ((msb << 5) | (lsb >> 3)) & 0xFFF;
}

// Perform a 16 clocks-per-conversion measurement.
void tscMeasure(u32 command, u16 *buffer, u32 count)
{
    if (!count) return;

    u32 oldIME = enterCriticalSection();

    spiWaitBusy();
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC | SPI_CONTINUOUS;

    spiWrite(command);

    __tscReadToBuffer(command, buffer, count);

    leaveCriticalSection(oldIME);
}

bool tscTouchReadData(touchRawArray *data)
{
    int oldIME = enterCriticalSection();

    // Hold ADC on. We're reading at near-full speed, and this
    // may slightly improve read accuracy.

    // Skip the first sample; to achieve this, we read measurements
    // into the *preceding* halfword for all values.
    spiWaitBusy();
    __tscMeasureFiveSkipFirst(TSC_MEASURE_Z1 | TSC_POWER_ON, &data->z1[0]);
    __tscMeasureFiveSkipFirst(TSC_MEASURE_Z2 | TSC_POWER_ON, &data->z2[0]);
    __tscMeasureFiveSkipFirst(TSC_MEASURE_X | TSC_POWER_ON, &data->rawX[0]);
    __tscMeasureFiveSkipFirst(TSC_MEASURE_Y | TSC_POWER_ON, &data->rawY[0]);

    // Make an empty read to switch the TSC into power-down mode.
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC | SPI_CONTINUOUS;
    spiWrite(TSC_MEASURE_TEMP1 | TSC_POWER_AUTO);
    spiRead();
    REG_SPICNT = SPI_ENABLE | SPI_TARGET_TSC;
    spiRead();

    REG_SPICNT = 0;

    leaveCriticalSection(oldIME);

    return true;
}

/// The constant 273.15, expressed in 20.12 fixed point.
#define KELVIN_CELSIUS_DIFF_20_12 1118822

// Deriving the delta multiplier:
//
// T = ([electron charge] * [voltage delta]) / ([Boltzmann constant] * ln([voltage ratio]))
// voltage ratio (of TEMP2 relative to TEMP1) = 91
//
// T = 2.573 * [voltage delta] mV
// voltage delta (V) = (TEMP2 - TEMP1) * Vref / 4096
// ... on NDS, Vref (V) ~= 3.3
//
// T = 2.573 * 3.3 / 4096 * 1000 * (TEMP2 - TEMP1)
//
// T = 2.073 * (TEMP2 - TEMP1)
//
// T (20.12) = 2.073 * 4096 * (TEMP2 - TEMP1)
// T (20.12) = 8490 * (TEMP2 - TEMP1)
#define TEMPERATURE_DELTA_MULTIPLIER 8490

s32 tscReadTemperature(void)
{
    // TODO: Should some kind of noise filter be used here too?
    u16 temp1 = tscRead(TSC_MEASURE_TEMP1 | TSC_POWER_ON);
    u16 temp2 = tscRead(TSC_MEASURE_TEMP2 | TSC_POWER_AUTO);
    return TEMPERATURE_DELTA_MULTIPLIER * (temp2 - temp1) - KELVIN_CELSIUS_DIFF_20_12;
}
