// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (Dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2005 Chris Double (doublec)

#include "nds/arm7/serial.h"
#include "nds/arm7/tsc.h"
#include "nds/system.h"
#include <nds/arm7/audio.h>
#include <nds/arm7/codec.h>
#include <nds/fifocommon.h>
#include <nds/interrupts.h>
#include <nds/timers.h>

// Microphone code based on neimod's microphone example.
//
// See: http://neimod.com/dstek/
// Chris Double (chris.double@double.co.nz)
// http://www.double.co.nz/nintendo_ds

void micSetAmp_TWL(u8 control, u8 gain);
s16 micReadData16_TWL(void);

// Turn on the Microphone Amp. Code based on neimod's example.
void micSetAmp_NTR(u8 control, u8 gain)
{
    spiWaitBusy();

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_POWER | SPI_CONTINUOUS;
    spiWrite(PM_AMP_OFFSET);

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_POWER;
    spiWrite(control);

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_POWER | SPI_CONTINUOUS;
    spiWrite(PM_GAIN_OFFSET);

    REG_SPICNT = SPI_ENABLE | SPI_TARGET_POWER;
    spiWrite(gain);
}

static inline u8 micReadData8_NTR(void)
{
    return tscRead(TSC_MEASURE_AUX | TSC_CONVERT_8BIT) >> 4;
}

static inline u16 micReadData12_NTR(void)
{
    return tscRead(TSC_MEASURE_AUX | TSC_CONVERT_12BIT);
}

void micSetAmp(u8 control, u8 gain)
{
    int oldIME = enterCriticalSection();

    if (cdcIsAvailable())
        micSetAmp_TWL(control, gain);
    else
        micSetAmp_NTR(control, gain);

    leaveCriticalSection(oldIME);
}

u8 micReadData8(void)
{
    u8 smp;
    int oldIME = enterCriticalSection();

    if (cdcIsAvailable())
        smp = (micReadData16_TWL() >> 8) ^ 0x80;
    else
        smp = micReadData8_NTR();

    leaveCriticalSection(oldIME);
    return smp;
}

u16 micReadData12(void)
{
    u16 smp;
    int oldIME = enterCriticalSection();

    if (cdcIsAvailable())
        smp = (micReadData16_TWL() ^ 0x8000) >> 4;
    else
        smp = micReadData12_NTR();

    leaveCriticalSection(oldIME);
    return smp;
}

s16 micReadData16(void)
{
    s16 smp;
    int oldIME = enterCriticalSection();

    if (cdcIsAvailable())
        smp = micReadData16_TWL();
    else
        smp = (micReadData12_NTR() << 4) ^ 0x8000;

    leaveCriticalSection(oldIME);
    return smp;
}

static u8* microphone_front_buffer;
static u8* microphone_back_buffer;
static int microphone_buffer_length = 0;
static int sampleCount = 0;
static bool eightBit = true;
static int micTimer = 0;
static MIC_BUF_SWAP_CB swapCallback;

void micStartRecording(u8 *buffer, int length, int freq, int timer,
                       bool eightBitSample, MIC_BUF_SWAP_CB bufferSwapCallback)
{
    microphone_front_buffer = buffer + length / 2;
    microphone_back_buffer = buffer;
    microphone_buffer_length = length / 2;
    swapCallback = bufferSwapCallback;
    sampleCount = 0;
    micTimer = timer;
    eightBit = eightBitSample;
    micOn();

    irqSet(IRQ_TIMER(timer), micTimerHandler);
    irqEnable(IRQ_TIMER(timer));

    // Setup a timer
    TIMER_DATA(timer) = TIMER_FREQ(freq);
    TIMER_CR(timer) = TIMER_ENABLE | TIMER_IRQ_REQ;
}

int micStopRecording(void)
{
    TIMER_CR(micTimer) &= ~TIMER_ENABLE;
    micOff();

    if (swapCallback)
        swapCallback(microphone_back_buffer, eightBit ? sampleCount : (sampleCount << 1));

    microphone_front_buffer = microphone_back_buffer = 0;

    return sampleCount;
}

void micTimerHandler(void)
{
    int len = 0;

    // Read data from the microphone. Data from the mic is unsigned, flipping
    // the highest bit makes it signed.

    if (eightBit)
    {
        *(microphone_back_buffer+sampleCount) = micReadData8() ^ 0x80;
    }
    else
    {
        *(s16*)(microphone_back_buffer + sampleCount * 2) = micReadData16();
    }

    sampleCount++;

    len = eightBit ? sampleCount : (sampleCount << 1);

    if (len >= microphone_buffer_length)
    {
        sampleCount = 0;

        u8 *temp = microphone_back_buffer;
        microphone_back_buffer = microphone_front_buffer;
        microphone_front_buffer = temp;

        if (swapCallback)
            swapCallback(microphone_front_buffer, microphone_buffer_length);
    }
}
