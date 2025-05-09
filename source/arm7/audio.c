// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008-2010 Dave Murphy (WinterMute)
// Copyright (C) 2008-2010 Jason Rogers (dovoto)

#include <nds/arm7/audio.h>
#include <nds/arm7/codec.h>
#include <nds/dma.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/ipc.h>
#include <nds/system.h>

#include "arm7/libnds_internal.h"

int getFreeChannel(void)
{
    for (int i = 0; i < 16; i++)
    {
        if (!(SCHANNEL_CR(i) & SCHANNEL_ENABLE))
            return i;
    }

    return -1;
}

int getFreePSGChannel(void)
{
    for (int i = 8; i < 14; i++)
    {
        if (!(SCHANNEL_CR(i) & SCHANNEL_ENABLE))
            return i;
    }

    return -1;
}

int getFreeNoiseChannel(void)
{
    for (int i = 14; i < 16; i++)
    {
        if (!(SCHANNEL_CR(i) & SCHANNEL_ENABLE))
            return i;
    }

    return -1;
}

void micSwapHandler(u8 *buffer, int length)
{
    FifoMessage msg;
    msg.type = MIC_BUFFER_FULL_MESSAGE;
    msg.MicBufferFull.buffer = (void *)buffer;
    msg.MicBufferFull.length = (u32)length;

    fifoSendDatamsg(FIFO_SOUND, sizeof(msg), (u8 *)&msg);
}

void soundDataHandler(int bytes, void *user_data)
{
    (void)user_data;

    int channel = -1;

    FifoMessage msg;

    fifoGetDatamsg(FIFO_SOUND, bytes, (u8 *)&msg);

    if (msg.type == SOUND_PLAY_MESSAGE)
    {
        channel = msg.SoundPlay.channel;

        // If the user wants libnds to look for a free channel
        if (channel < 0)
            channel = getFreeChannel();

        // If the user-specified channel is invalid or all channels are busy
        if (channel > 15)
            channel = -1;

        if (channel >= 0)
        {
            SCHANNEL_SOURCE(channel) = (u32)msg.SoundPlay.data;
            SCHANNEL_REPEAT_POINT(channel) = msg.SoundPlay.loopPoint;
            SCHANNEL_LENGTH(channel) = msg.SoundPlay.dataSize;
            SCHANNEL_TIMER(channel) = SOUND_FREQ(msg.SoundPlay.freq);
            SCHANNEL_CR(channel) = SCHANNEL_ENABLE | SOUND_VOL(msg.SoundPlay.volume)
                                   | SOUND_PAN(msg.SoundPlay.pan)
                                   | (msg.SoundPlay.format << 29)
                                   | (msg.SoundPlay.loop ?  SOUND_REPEAT : SOUND_ONE_SHOT);
        }
    }
    else if (msg.type == SOUND_PSG_MESSAGE)
    {
        channel = msg.SoundPsg.channel;

        // If the user wants libnds to look for a free channel
        if (channel < 0)
            channel = getFreePSGChannel();

        // If the user-specified channel is invalid or all channels are busy
        if (channel < 8 || channel > 13)
            channel = -1;

        if (channel >= 0)
        {
            SCHANNEL_CR(channel) = SCHANNEL_ENABLE | msg.SoundPsg.volume
                                   | SOUND_PAN(msg.SoundPsg.pan) | SOUND_FORMAT_PSG
                                   | (msg.SoundPsg.dutyCycle << 24);
            SCHANNEL_TIMER(channel) = SOUND_FREQ(msg.SoundPsg.freq);
        }
    }
    else if (msg.type == SOUND_NOISE_MESSAGE)
    {
        channel = msg.SoundPsg.channel;

        // If the user wants libnds to look for a free channel
        if (channel < 0)
            channel = getFreeNoiseChannel();

        // If the user-specified channel is invalid or all channels are busy
        if (channel < 14 || channel > 15)
            channel = -1;

        if (channel >= 0)
        {
            SCHANNEL_CR(channel) = SCHANNEL_ENABLE | msg.SoundPsg.volume
                                   | SOUND_PAN(msg.SoundPsg.pan) | SOUND_FORMAT_PSG;
            SCHANNEL_TIMER(channel) = SOUND_FREQ(msg.SoundPsg.freq);
        }
    }
    else if (msg.type == SOUND_CAPTURE_START)
    {
        u8 value = SNDCAPCNT_START_BUSY;

        if (msg.SoundCaptureStart.repeat == 0)
            value |= SNDCAPCNT_ONESHOT;
        if (msg.SoundCaptureStart.format)
            value |= SNDCAPCNT_FORMAT_8BIT;

        if (msg.SoundCaptureStart.sndcapChannel == 0)
        {
            REG_SNDCAP0DAD = (u32)msg.SoundCaptureStart.buffer;
            REG_SNDCAP0LEN = msg.SoundCaptureStart.bufferLen;

            if (msg.SoundCaptureStart.addCapToChannel)
                value |= SND0CAPCNT_CH1_OUT_ADD_TO_CH0;
            if (msg.SoundCaptureStart.sourceIsMixer == 0)
                value |= SND0CAPCNT_SOURCE_CH0;

            REG_SNDCAP0CNT = value;

            channel = 0;
        }
        else if (msg.SoundCaptureStart.sndcapChannel == 1)
        {
            REG_SNDCAP1DAD = (u32)msg.SoundCaptureStart.buffer;
            REG_SNDCAP1LEN = msg.SoundCaptureStart.bufferLen;

            if (msg.SoundCaptureStart.addCapToChannel)
                value |= SND1CAPCNT_CH3_OUT_ADD_TO_CH2;
            if (msg.SoundCaptureStart.sourceIsMixer == 0)
                value |= SND1CAPCNT_SOURCE_CH2;

            REG_SNDCAP1CNT = value;

            channel = 1;
        }
    }
    else if (msg.type == MIC_RECORD_MESSAGE)
    {
        micStartRecording(msg.MicRecord.buffer, msg.MicRecord.bufferLength,
                          msg.MicRecord.freq, 1, msg.MicRecord.format, micSwapHandler);

        channel = 17;
    }

    fifoSendValue32(FIFO_SOUND, (u32)channel);
}

void enableSound(void)
{
    powerOn(POWER_SOUND);

    // DS Power Management Device: Disable mute bit and enable amplifier
    writePowerManagement(PM_CONTROL_REG,
            (readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE) | PM_SOUND_AMP);

    REG_SOUNDCNT = SOUND_ENABLE;

    if (isDSiMode())
    {
        // Enabled, not muted, 100% ARM output
        REG_SNDEXTCNT = (REG_SNDEXTCNT & ~SNDEXTCNT_RATIO(0xF)) | SNDEXTCNT_ENABLE | SNDEXTCNT_RATIO(8);
        // 32 kHz I2S frequency
        soundExtSetFrequencyTWL(32);
    }

    REG_MASTER_VOLUME = 127;

    // Clear sound registers
    dmaFillWords(0, (void *)0x04000400, 0x100);
}

void disableSound(void)
{
    REG_SOUNDCNT &= ~SOUND_ENABLE;

    if (isDSiMode())
        REG_SNDEXTCNT &= ~SNDEXTCNT_ENABLE;

    writePowerManagement(PM_CONTROL_REG,
            (readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_AMP) | PM_SOUND_MUTE);
    powerOff(POWER_SOUND);
}

void soundCommandHandler(u32 command, void *userdata)
{
    (void)userdata;

    int cmd = command & 0x00F00000;
    int data = command & 0xFFFF;
    int channel = (command >> 16) & 0xF;

    switch (cmd)
    {
        case SOUND_MASTER_ENABLE:
            enableSound();
            break;

        case SOUND_MASTER_DISABLE:
            disableSound();
            break;

        case SOUND_SET_VOLUME:
            SCHANNEL_CR(channel) &= ~0xFF;
            SCHANNEL_CR(channel) |= data;
            break;

        case SOUND_SET_PAN:
            SCHANNEL_CR(channel) &= ~SOUND_PAN(0xFF);
            SCHANNEL_CR(channel) |= SOUND_PAN(data);
            break;

        case SOUND_SET_FREQ:
            SCHANNEL_TIMER(channel) = SOUND_FREQ(data);
            break;

        case SOUND_SET_WAVEDUTY:
            SCHANNEL_CR(channel) &= ~(7 << 24);
            SCHANNEL_CR(channel) |= (data) << 24;
            break;

        case SOUND_KILL:
            SCHANNEL_CR(channel) &= ~SCHANNEL_ENABLE;
            break;

        case SOUND_PAUSE:
            SCHANNEL_CR(channel) &= ~SCHANNEL_ENABLE;
            break;

        case SOUND_RESUME:
            SCHANNEL_CR(channel) |= SCHANNEL_ENABLE;
            break;

        case SOUND_CAPTURE_STOP:
            if (channel == 0)
                REG_SNDCAP0CNT = SNDCAPCNT_STOP;
            else if (channel == 1)
                REG_SNDCAP1CNT = SNDCAPCNT_STOP;
            break;

        case MIC_SET_POWER_ON:
            if (!isDSiMode())
                break;

            REG_MICCNT = 0; // Disable sending samples to ARM7 registers

            if (data != 0)
                micOn();
            else
                micOff();

            break;

        case MIC_STOP:
            micStopRecording();
            break;

        case SOUND_EXT_SET_FREQ:
            soundExtSetFrequencyTWL(data);
            break;

        case SOUND_EXT_SET_RATIO:
            if (!isDSiMode())
                break;

            if (data > 8)
                data = 8;

            // The ratio can be changed even if the enable bit is set to 1
            REG_SNDEXTCNT &= ~SNDEXTCNT_RATIO(0xF);
            REG_SNDEXTCNT |= SNDEXTCNT_RATIO(data);
            break;

        default:
            break;
    }
}

void installSoundFIFO(void)
{
    fifoSetDatamsgHandler(FIFO_SOUND, soundDataHandler, 0);
    fifoSetValue32Handler(FIFO_SOUND, soundCommandHandler, 0);
}

bool soundExtSetFrequencyTWL(unsigned int freq_khz)
{
    if (!isDSiMode())
        return false;

    if (!cdcIsAvailable())
        return false;

    return twlSoundExtSetFrequency(freq_khz);
}
