// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008-2010 Dave Murphy (WinterMute)
// Copyright (C) 2008-2010 Jason Rogers (Dovoto)

#include <nds/arm7/audio.h>
#include <nds/dma.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/ipc.h>
#include <nds/system.h>

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
    msg.MicBufferFull.buffer = (void*)buffer;
    msg.MicBufferFull.length = (u32)length;

    fifoSendDatamsg(FIFO_SOUND, sizeof(msg), (u8 *)&msg);
}

void soundDataHandler(int bytes, void *user_data)
{
    (void)user_data;

    int channel = -1;

    FifoMessage msg;

    fifoGetDatamsg(FIFO_SOUND, bytes, (u8*)&msg);

    if (msg.type == SOUND_PLAY_MESSAGE)
    {
        channel = getFreeChannel();

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
        channel = getFreePSGChannel();

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
        channel = getFreeNoiseChannel();

        if (channel >= 0)
        {
            SCHANNEL_CR(channel) = SCHANNEL_ENABLE | msg.SoundPsg.volume
                                 | SOUND_PAN(msg.SoundPsg.pan) | SOUND_FORMAT_PSG;
            SCHANNEL_TIMER(channel) = SOUND_FREQ(msg.SoundPsg.freq);
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
    REG_MASTER_VOLUME = 127;

    // Clear sound registers
    dmaFillWords(0, (void *)0x04000400, 0x100);
}

void disableSound(void)
{
    REG_SOUNDCNT &= ~SOUND_ENABLE;
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

        case MIC_STOP:
            micStopRecording();
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
