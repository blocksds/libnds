// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008 Jason Rogers (dovoto)
// Copyright (C) 2008 Dave Murphy (WinterMute)

// Sound Functions

#include <string.h>

#include <nds/arm9/cache.h>
#include <nds/arm9/sassert.h>
#include <nds/arm9/sound.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <nds/system.h>

void soundEnable(void)
{
    fifoSendValue32(FIFO_SOUND, SOUND_MASTER_ENABLE);
}

void soundDisable(void)
{
    fifoSendValue32(FIFO_SOUND, SOUND_MASTER_DISABLE);
}

void soundSetMasterVolume(u32 volume)
{
    if (volume > 127)
        volume = 127;

    fifoSendValue32(FIFO_SOUND, SOUND_SET_MASTER_VOL | volume);
}

int soundPlayPSGChannel(int channel, DutyCycle cycle, u16 freq, u8 volume, u8 pan)
{
    FifoMessage msg;

    msg.type = SOUND_PSG_MESSAGE;
    msg.SoundPsg.channel = channel;
    msg.SoundPsg.dutyCycle = cycle;
    msg.SoundPsg.freq = freq;
    msg.SoundPsg.volume = volume;
    msg.SoundPsg.pan = pan;

    fifoMutexAcquire(FIFO_SOUND);

    fifoSendDatamsg(FIFO_SOUND, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32Async(FIFO_SOUND);
    int result = fifoGetValue32(FIFO_SOUND);

    fifoMutexRelease(FIFO_SOUND);

    return result;
}

int soundPlayNoiseChannel(int channel, u16 freq, u8 volume, u8 pan)
{
    FifoMessage msg;

    msg.type = SOUND_NOISE_MESSAGE;
    msg.SoundPsg.channel = channel;
    msg.SoundPsg.freq = freq;
    msg.SoundPsg.volume = volume;
    msg.SoundPsg.pan = pan;

    fifoMutexAcquire(FIFO_SOUND);

    fifoSendDatamsg(FIFO_SOUND, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32Async(FIFO_SOUND);
    int result = fifoGetValue32(FIFO_SOUND);

    fifoMutexRelease(FIFO_SOUND);

    return result;
}

int soundPlaySampleChannel(int channel, const void *data, SoundFormat format,
                           u32 dataSize, u16 freq, u8 volume, u8 pan,
                           bool loop, u16 loopPoint)
{
    FifoMessage msg;

    msg.type = SOUND_PLAY_MESSAGE;
    msg.SoundPlay.channel = channel;
    msg.SoundPlay.data = data;
    msg.SoundPlay.freq = freq;
    msg.SoundPlay.volume = volume;
    msg.SoundPlay.pan = pan;
    msg.SoundPlay.loop = loop;
    msg.SoundPlay.format = format;
    msg.SoundPlay.loopPoint = loopPoint;
    msg.SoundPlay.dataSize = dataSize >> 2;

    fifoMutexAcquire(FIFO_SOUND);

    fifoSendDatamsg(FIFO_SOUND, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32Async(FIFO_SOUND);
    int result = fifoGetValue32(FIFO_SOUND);

    fifoMutexRelease(FIFO_SOUND);

    return result;
}

void soundPause(int soundId)
{
    fifoSendValue32(FIFO_SOUND, SOUND_PAUSE | (soundId << 16));
}

void soundKill(int soundId)
{
    fifoSendValue32(FIFO_SOUND, SOUND_KILL | (soundId << 16));
}

void soundResume(int soundId)
{
    fifoSendValue32(FIFO_SOUND, SOUND_RESUME | (soundId << 16));
}

void soundSetVolume(int soundId, u8 volume)
{
    fifoSendValue32(FIFO_SOUND, SOUND_SET_VOLUME | (soundId << 16) | volume);
}

void soundSetPan(int soundId, u8 pan)
{
    fifoSendValue32(FIFO_SOUND, SOUND_SET_PAN | (soundId << 16) | pan);
}

void soundSetFreq(int soundId, u16 freq)
{
    fifoSendValue32(FIFO_SOUND, SOUND_SET_FREQ | (soundId << 16) | freq);
}

void soundSetWaveDuty(int soundId, DutyCycle cycle)
{
    fifoSendValue32(FIFO_SOUND, SOUND_SET_WAVEDUTY | (soundId << 16) | cycle);
}

int soundCaptureStart(void *buffer, u16 bufferLen, int sndcapChannel,
                      bool addCapToChannel, bool sourceIsMixer, bool repeat,
                      SoundCaptureFormat format)
{
    FifoMessage msg;

    msg.type = SOUND_CAPTURE_START;
    msg.SoundCaptureStart.buffer = buffer;
    msg.SoundCaptureStart.bufferLen = bufferLen;
    msg.SoundCaptureStart.sndcapChannel = sndcapChannel;
    msg.SoundCaptureStart.addCapToChannel = addCapToChannel;
    msg.SoundCaptureStart.sourceIsMixer = sourceIsMixer;
    msg.SoundCaptureStart.repeat = repeat;
    msg.SoundCaptureStart.format = format;

    fifoMutexAcquire(FIFO_SOUND);

    fifoSendDatamsg(FIFO_SOUND, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32Async(FIFO_SOUND);
    int result = fifoGetValue32(FIFO_SOUND);

    fifoMutexRelease(FIFO_SOUND);

    return result;
}

void soundCaptureStop(int sndcapChannel)
{
    fifoSendValue32(FIFO_SOUND, SOUND_CAPTURE_STOP | (sndcapChannel << 16));
}

MicCallback micCallback = 0;

void micBufferHandler(int bytes, void *user_data)
{
    (void)user_data;

    FifoMessage msg;

    fifoGetDatamsg(FIFO_SOUND, bytes, (u8 *)&msg);

    if (msg.type == MIC_BUFFER_FULL_MESSAGE)
    {
        if (micCallback)
            micCallback(msg.MicBufferFull.buffer, msg.MicBufferFull.length);
    }
}

int soundMicRecord(void *buffer, u32 bufferLength, MicFormat format, int freq,
                   MicCallback callback)
{
    FifoMessage msg;

    msg.type = MIC_RECORD_MESSAGE;
    msg.MicRecord.format = format;
    msg.MicRecord.buffer = buffer;
    msg.MicRecord.freq = freq;
    msg.MicRecord.bufferLength = bufferLength;

    micCallback = callback;

    fifoSetDatamsgHandler(FIFO_SOUND, micBufferHandler, 0);

    fifoMutexAcquire(FIFO_SOUND);

    fifoSendDatamsg(FIFO_SOUND, sizeof(msg), (u8 *)&msg);
    fifoWaitValue32Async(FIFO_SOUND);
    int result = fifoGetValue32(FIFO_SOUND);

    fifoMutexRelease(FIFO_SOUND);

    return result;
}

void soundMicOff(void)
{
    fifoSendValue32(FIFO_SOUND, MIC_STOP);
}

void soundExtSetFrequency(unsigned int freq_khz)
{
    if (!isDSiMode())
        return;

    sassert((freq_khz == 47) || (freq_khz == 32),
            "Frequency must be 32 or 47 (KHz)");

    fifoSendValue32(FIFO_SOUND, SOUND_EXT_SET_FREQ | freq_khz);
}

void soundExtSetRatio(unsigned int ratio)
{
    if (!isDSiMode())
        return;

    if (ratio > 8)
        ratio = 8;

    fifoSendValue32(FIFO_SOUND, SOUND_EXT_SET_RATIO | ratio);
}

void soundMicPowerOn(void)
{
    if (!isDSiMode())
        return;

    fifoSendValue32(FIFO_SOUND, MIC_SET_POWER_ON | 1);
}

void soundMicPowerOff(void)
{
    if (!isDSiMode())
        return;

    fifoSendValue32(FIFO_SOUND, MIC_SET_POWER_ON | 0);
}
