// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 fincs

// DSi "codec" Touchscreen/Sound Controller control for ARM7

#include "nds/arm7/codec.h"
#include "nds/arm7/serial.h"
#include "nds/arm7/touch.h"
#include "nds/interrupts.h"

static u8 readTSC(u8 reg)
{
    spiWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_CODEC | SPI_CONTINUOUS;
    spiWrite(1 | (reg << 1));

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_CODEC;
    return spiRead();
}

static void writeTSC(u8 reg, u8 value)
{
    spiWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_CODEC | SPI_CONTINUOUS;
    spiWrite(reg << 1);

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_CODEC;
    spiWrite(value);
}

static void bankSwitchTSC(u8 bank)
{
    static u8 curBank = 0x63;
    if (bank != curBank)
    {
        writeTSC(curBank == 0xFF ? 0x7F : 0x00, bank);
        curBank = bank;
    }
}

u8 cdcReadReg(u8 bank, u8 reg)
{
    bankSwitchTSC(bank);
    return readTSC(reg);
}

void cdcReadRegArray(u8 bank, u8 reg, void *data, u8 size)
{
    u8 *out = (u8 *)data;
    bankSwitchTSC(bank);

    spiWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_CODEC | SPI_CONTINUOUS;
    spiWrite(1 | (reg << 1));

    for (; size > 1; size--)
        *out++ = spiRead();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_CODEC;
    *out++ = spiRead();
}

u16 cdcReadReg16(u8 bank, u8 reg)
{
    u8 data[2];
    cdcReadRegArray(bank, reg, data, 2);
    return (data[0] << 8) | data[1];
}

u32 cdcReadReg24(u8 bank, u8 reg)
{
    u8 data[3];
    cdcReadRegArray(bank, reg, data, 3);
    return (data[0] << 16) | (data[1] << 8) | data[2];
}

void cdcWriteReg(u8 bank, u8 reg, u8 value)
{
    bankSwitchTSC(bank);
    writeTSC(reg, value);
}

void cdcWriteRegMask(u8 bank, u8 reg, u8 mask, u8 value)
{
    bankSwitchTSC(bank);
    writeTSC(reg, (readTSC(reg) & ~mask) | (value & mask));
}

void cdcWriteRegArray(u8 bank, u8 reg, const void *data, u8 size)
{
    const u8 *in = data;
    bankSwitchTSC(bank);

    spiWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_CODEC | SPI_CONTINUOUS;
    spiWrite(reg << 1);

    for (; size > 1; size--)
        spiWrite(*in++);

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_CODEC;
    spiWrite(*in++);
}

void cdcWriteReg16(u8 bank, u8 reg, u16 value)
{
    u8 data[2];
    data[0] = value >> 8;
    data[1] = value;
    cdcWriteRegArray(bank, reg, data, 2);
}

void cdcWriteReg24(u8 bank, u8 reg, u32 value)
{
    u8 data[3];
    data[0] = value >> 16;
    data[1] = value >> 8;
    data[2] = value;
    cdcWriteRegArray(bank, reg, data, 3);
}

void cdcTouchInit(void)
{
    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_TWL_PEN_DOWN,
                    CDC_TOUCHCNT_TWL_PEN_DOWN_ENABLE, 0);
    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_SAR_ADC_CTRL1,
                    CDC_TOUCHCNT_SAR_ADC_CLOCK_DIV_MASK, CDC_TOUCHCNT_SAR_ADC_CLOCK_DIV_8);
    cdcWriteReg(CDC_TOUCHCNT, CDC_TOUCHCNT_SCAN_MODE_TIMER, 0xA0);
    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_TWL_PEN_DOWN, 0x38, 5 << 3);
    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_TWL_PEN_DOWN, 0x40, 0 << 6);

    // TODO: The IRQ value used should be documented.
    cdcWriteReg(CDC_TOUCHCNT, CDC_TOUCHCNT_SAR_ADC_CTRL2,
                CDC_TOUCHCNT_SAR_ADC_CONVERSION_SELF | CDC_TOUCHCNT_SAR_ADC_SCAN_XYZ | 3);

    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_PANEL_VOLTAGE_STABILIZATION,
                    CDC_TOUCHCNT_PANEL_VOLTAGE_STABILIZATION_TIME_MASK,
                    CDC_TOUCHCNT_PANEL_VOLTAGE_STABILIZATION_TIME_30US);
    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_PRECHARGE_SENSE,
                    CDC_TOUCHCNT_SENSE_TIME_MASK, CDC_TOUCHCNT_SENSE_TIME_300US);
    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_PRECHARGE_SENSE,
                    CDC_TOUCHCNT_PRECHARGE_TIME_MASK, CDC_TOUCHCNT_PRECHARGE_TIME_30US);
    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_DEBOUNCE_PENUP,
                    CDC_TOUCHCNT_DEBOUNCE_TIME_MASK, CDC_TOUCHCNT_DEBOUNCE_TIME_0US);
    cdcWriteRegMask(CDC_TOUCHCNT, CDC_TOUCHCNT_TWL_PEN_DOWN,
                    CDC_TOUCHCNT_TWL_PEN_DOWN_ENABLE, CDC_TOUCHCNT_TWL_PEN_DOWN_ENABLE);
}

bool cdcTouchPenDown(void)
{
    return (cdcReadReg(CDC_TOUCHCNT, CDC_TOUCHCNT_STATUS) & 0xC0) != 0x40
        && !(cdcReadReg(CDC_TOUCHCNT, CDC_TOUCHCNT_TWL_PEN_DOWN) & 0x02);
}

_Static_assert(sizeof(touchRawArray) == 40, "Incompatible struct size!");

bool cdcTouchReadData(touchRawArray *data)
{
    int oldIME = enterCriticalSection();
    cdcReadRegArray(CDC_TOUCHDATA, 0x01, &data->rawX[0], 20 * sizeof(u16));
    leaveCriticalSection(oldIME);

    // "data" consists of 20 halfwords; five for X, five for Y,
    // five for Z1, five for Z2. However, they are endianness
    // swapped - let's fix that in a bulk operation for faster
    // performance.
    //
    // For this to work, data->rawX has to be word-aligned.
    u32 *wordsToSwap = (u32 *) &data->rawX[0];
    for (int i = 0; i < 10; i++, wordsToSwap++)
    {
        u32 tmp = *wordsToSwap;

        // The halfwords should be 12-bit; if the highest four bits
        // (byte 0, byte 2 before swapping) are set, the readout is
        // invalid.
        if (tmp & 0x00F000F0)
            return false;

        *wordsToSwap = ((tmp & 0xFF00FF) << 8) | ((tmp & 0xFF00FF00) >> 8);
    }

    return true;
}
