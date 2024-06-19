// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#include <stdbool.h>
#include <nds/arm7/audio.h>
#include <nds/arm7/codec.h>
#include <nds/system.h>

/// Enable low-power TSC2117 clock divider configuration.
/// This disables the PLL in favor of simple dividers; however, returning to
/// other homebrew programs or official code requires restoring the PLL-enabled
/// state, as their changes between 47 and 32 kHz only modify PLL/DAC/ADC clock
/// multiplier/divider configuration.
///
/// TODO: Consider adding way to re-intiialize clock divider configuration when
///       returning to loader.
/// TODO: Further testing.
// #define TSC2117_LOW_POWER_DIVIDERS_ENABLE

bool soundExtSetFrequencyTWL(unsigned int freq_khz)
{
    bool is_high_freq = freq_khz >= 47;

    if (!isDSiMode())
        return false;
    if (!cdcIsAvailable())
        return false;

    // Check if frequency already set
    bool is_current_high_freq = (REG_SNDEXTCNT & SNDEXTCNT_FREQ_47KHZ);
    if (is_high_freq == is_current_high_freq)
        return true;

    // Disable I2S output if enabled
    bool previously_enabled = REG_SNDEXTCNT & SNDEXTCNT_ENABLE;
    if (previously_enabled)
        REG_SNDEXTCNT &= ~SNDEXTCNT_ENABLE;

#ifdef TSC2117_LOW_POWER_DIVIDERS_ENABLE
    // Reconfigure clock dividers, based on the TSC2117 datasheet.
    // - We disable PLL, as MCLK is always equal to the sample frequency
    //   times 256, which is an integer multiple.
    // - We disable ADC NADC/MADC dividers, to share the DAC clock.
    // This also prevents us from having to reconfigure the PLL multipliers
    // for 32kHz/47kHz.
    cdcWriteReg(CDC_CONTROL, CDC_CONTROL_PLL_PR, 0);
    cdcWriteReg(CDC_CONTROL, CDC_CONTROL_DAC_MDAC, CDC_CONTROL_CLOCK_ENABLE(2));
    cdcWriteReg(CDC_CONTROL, CDC_CONTROL_DAC_NDAC, CDC_CONTROL_CLOCK_ENABLE(1));
    cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_MADC, CDC_CONTROL_CLOCK_DISABLE);
    cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_NADC, CDC_CONTROL_CLOCK_DISABLE);
    cdcWriteReg(CDC_CONTROL, CDC_CONTROL_CLOCK_MUX, CDC_CONTROL_CLOCK_PLL_IN_MCLK | CDC_CONTROL_CLOCK_CODEC_IN_MCLK);
#else
    // Reconfigure clock dividers, based on the TSC2117 datasheet.
    // Assume the default clock signal path - only reconfigure the PLL
    // multiplier and respective DAC/ADC dividers.

    // Match the ADC clock divider with the DAC clock divider.
    //
    // TODO: Is it better to power down ADC NADC/MADC dividers in this path?
    //       SNDEXCNT doesn't allow configuring the clock separately, so
    //       they always match the DAC ones. The datasheet claims it's fine
    //       (disabling makes them share the DAC clock).
    cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_MADC, CDC_CONTROL_CLOCK_DISABLE);
    cdcWriteReg(CDC_CONTROL, CDC_CONTROL_ADC_NADC, CDC_CONTROL_CLOCK_DISABLE);

    // The PLL clock has to be between the 80 and 110 MHz range; changing
    // SNDEXCNT changes MCLK, so the PLL multiplier/dividers have to follow:
    // - 32KHz: MCLK = BUS_CLOCK / 4 ~= 8.38 MHz
    //          PLL_CLK = MCLK * 21 / 2 ~= 87.97 MHz
    // - 47KHz: BUS_CLOCK * 4 / 11 ~= 12.19 MHz
    //          PLL_CLK = MCLK * 15 / 2 ~= 91.40 MHz
    // The NDAC divider has to be adjusted to result in a matched ratio;
    // the output frequency is always MCLK / 256.
    if (is_high_freq)
    {
        // Configure a PLL multiplier/divider of 15/2, and a NDAC/NADC divider of 5.
        cdcWriteReg(CDC_CONTROL, CDC_CONTROL_PLL_J, 15);
        cdcWriteReg(CDC_CONTROL, CDC_CONTROL_DAC_NDAC, CDC_CONTROL_CLOCK_ENABLE(5));
    }
    else
    {
        // Configure a PLL multiplier/divider of 21/2, and a NDAC/NADC divider of 7.
        cdcWriteReg(CDC_CONTROL, CDC_CONTROL_DAC_NDAC, CDC_CONTROL_CLOCK_ENABLE(7));
        cdcWriteReg(CDC_CONTROL, CDC_CONTROL_PLL_J, 21);
    }
#endif

    // Configure and enable I2S output
    REG_SNDEXTCNT = (REG_SNDEXTCNT & ~SNDEXTCNT_FREQ_47KHZ) | (is_high_freq ? SNDEXTCNT_FREQ_47KHZ : SNDEXTCNT_FREQ_32KHZ) | SNDEXTCNT_ENABLE;

    if (previously_enabled)
        REG_SNDEXTCNT |= SNDEXTCNT_ENABLE;

    return true;
}
