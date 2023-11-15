// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 fincs

// DSi "codec" Touchscreen/Sound Controller control for ARM7

#include <nds/arm7/codec.h>

static u8 readTSC(u8 reg)
{
    SerialWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
    REG_SPIDATA = 1 | (reg << 1);

    SerialWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH;
    REG_SPIDATA = 0;

    SerialWaitBusy();

    return REG_SPIDATA;
}

static void writeTSC(u8 reg, u8 value)
{
    SerialWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
    REG_SPIDATA = reg << 1;

    SerialWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH;
    REG_SPIDATA = value;
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

    SerialWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
    REG_SPIDATA = 1 | (reg << 1);

    SerialWaitBusy();

    for (; size > 1; size--)
    {
        REG_SPIDATA = 0;
        SerialWaitBusy();
        *out++ = REG_SPIDATA;
    }

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH;
    REG_SPIDATA = 0;

    SerialWaitBusy();

    *out++ = REG_SPIDATA;
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
    const u8 *in = (u8 *)data;
    bankSwitchTSC(bank);

    SerialWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH | SPI_CONTINUOUS;
    REG_SPIDATA = reg << 1;

    SerialWaitBusy();

    for (; size > 1; size--)
    {
        REG_SPIDATA = *in++;
        SerialWaitBusy();
    }

    REG_SPICNT = SPI_ENABLE | SPI_BAUD_4MHz | SPI_DEVICE_TOUCH;
    REG_SPIDATA = *in++;
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

bool cdcTouchRead(touchPosition *pos)
{
    u8 raw[4 * 2 * 5];
    u16 arrayX[5], arrayY[5], arrayZ1[5], arrayZ2[5];
    u32 sumX, sumY, sumZ1, sumZ2;

    cdcReadRegArray(CDC_TOUCHDATA, 0x01, raw, sizeof(raw));

    for (int i = 0; i < 5; i ++)
    {
        arrayX[i] = (raw[i * 2 + 0] << 8) | raw[i * 2 + 1];
        arrayY[i] = (raw[i * 2 + 10] << 8) | raw[i * 2 + 11];
        arrayZ1[i] = (raw[i * 2 + 20] << 8) | raw[i * 2 + 21];
        arrayZ2[i] = (raw[i * 2 + 30] << 8) | raw[i * 2 + 31];

        if ((arrayX[i] & 0xF000) || (arrayY[i] & 0xF000))
        {
            pos->rawx = 0;
            pos->rawy = 0;
            return false;
        }
    }

    // TODO: For now we just average all values without removing inaccurate values
    sumX = 0;
    sumY = 0;
    sumZ1 = 0;
    sumZ2 = 0;

    for (int i = 0; i < 5; i ++)
    {
        sumX += arrayX[i];
        sumY += arrayY[i];
        sumZ1 += arrayZ1[i];
        sumZ2 += arrayZ2[i];
    }

    pos->rawx = sumX / 5;
    pos->rawy = sumY / 5;
    pos->z1 = sumZ1 / 5;
    pos->z2 = sumZ2 / 5;

    return true;
}
