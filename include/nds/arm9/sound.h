// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_ARM9_SOUND_H__
#define LIBNDS_NDS_ARM9_SOUND_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/arm9/sound.h
///
/// @brief A simple sound playback library for the DS.
///
/// Provides functionality for starting and stopping sound effects from the ARM9
/// side as well as access to PSG and noise hardware. Maxmod should be used in
/// most music and sound effect situations.

#include <nds/ndstypes.h>

typedef void (* MicCallback)(void *completedBuffer, int length);

/// Sound formats used by the DS
typedef enum
{
    SoundFormat_16Bit = 1, ///< 16-bit PCM
    SoundFormat_8Bit = 0,  ///< 8-bit PCM
    SoundFormat_PSG = 3,   ///< PSG (Programmable Sound Generator)
    SoundFormat_ADPCM = 2  ///< IMA ADPCM compressed audio
} SoundFormat;

/// Sound formats used by the audio capture unit
typedef enum
{
    SoundCaptureFormat_16Bit = 0, ///< 16-bit PCM
    SoundCaptureFormat_8Bit = 1,  ///< 8-bit PCM
} SoundCaptureFormat;

/// Microphone recording formats DS
typedef enum
{
    MicFormat_8Bit = 1, ///< 8-bit PCM
    MicFormat_12Bit = 0 ///< 12-bit PCM
} MicFormat;

/// PSG Duty cycles used by the PSG hardware
typedef enum
{
    DutyCycle_0  = 7, ///< 0.0% duty cycle
    DutyCycle_12 = 0, ///< 12.5% duty cycle
    DutyCycle_25 = 1, ///< 25.0% duty cycle
    DutyCycle_37 = 2, ///< 37.5% duty cycle
    DutyCycle_50 = 3, ///< 50.0% duty cycle
    DutyCycle_62 = 4, ///< 62.5% duty cycle
    DutyCycle_75 = 5, ///< 75.0% duty cycle
    DutyCycle_87 = 6  ///< 87.5% duty cycle
} DutyCycle;

/// Enables Sound on the DS.
///
/// It should be called prior to attempting sound playback.
void soundEnable(void);

/// Disables Sound on the DS.
void soundDisable(void);

/// Plays a sound in the specified format at the specified frequency.
///
/// @param channel
///     Channel to be used for this sound. If -1 is used, libnds will select a
///     free channel and use it.
/// @param data
///     A pointer to the sound data.
/// @param format
///     The format of the data (only 16-bit and 8-bit pcm and ADPCM formats are
///     supported by this function).
/// @param dataSize
///     The size in bytes of the sound data.
/// @param freq
///     The frequency in Hz of the sample.
/// @param volume
///     The channel volume. 0 to 127 (min to max).
/// @param pan
///     The channel pan 0 to 127 (left to right with 64 being centered).
/// @param loop
///     If true, the sample will loop playing once then repeating starting at
///     the offset stored in loopPoint.
/// @param loopPoint
///     The offset for the sample loop to restart when repeating.
///
/// @return
///     An integer id coresponding to the channel of playback. This value can be
///     used to pause, resume, or kill the sound as well as adjust volume, pan,
///     and frequency
int soundPlaySampleChannel(int channel, const void *data, SoundFormat format,
                           u32 dataSize, u16 freq, u8 volume, u8 pan,
                           bool loop, u16 loopPoint);

/// Plays a sound in the specified format at the specified frequency.
///
/// libnds will select a free channel to play the sound.
///
/// @param data
///     A pointer to the sound data.
/// @param format
///     The format of the data (only 16-bit and 8-bit pcm and ADPCM formats are
///     supported by this function).
/// @param dataSize
///     The size in bytes of the sound data.
/// @param freq
///     The frequency in Hz of the sample.
/// @param volume
///     The channel volume. 0 to 127 (min to max).
/// @param pan
///     The channel pan 0 to 127 (left to right with 64 being centered).
/// @param loop
///     If true, the sample will loop playing once then repeating starting at
///     the offset stored in loopPoint.
/// @param loopPoint
///     The offset for the sample loop to restart when repeating.
///
/// @return
///     An integer id coresponding to the channel of playback. This value can be
///     used to pause, resume, or kill the sound as well as adjust volume, pan,
///     and frequency
static inline int soundPlaySample(const void *data, SoundFormat format,
                                  u32 dataSize, u16 freq, u8 volume, u8 pan,
                                  bool loop, u16 loopPoint)
{
    return soundPlaySampleChannel(-1, data, format, dataSize, freq, volume, pan,
                                  loop, loopPoint);
}

/// Pause a tone with the specified properties.
///
/// @param channel
///     Channel to be used for this sound. If -1 is used, libnds will select a
///     free channel and use it.
/// @param cycle
///     The DutyCycle of the sound wave.
/// @param freq
///     The frequency in Hz of the sample.
/// @param volume
///     The channel volume.  0 to 127 (min to max)
/// @param pan
///     The channel pan 0 to 127 (left to right with 64 being centered).
///
/// @return
///     An integer id coresponding to the channel of playback. This value can be
///     used to pause, resume, or kill the sound as well as adjust volume, pan,
///     and frequency.
int soundPlayPSGChannel(int channel, DutyCycle cycle, u16 freq, u8 volume, u8 pan);

/// Pause a tone with the specified properties.
///
/// libnds will select a free channel to play the sound.
///
/// @param cycle
///     The DutyCycle of the sound wave.
/// @param freq
///     The frequency in Hz of the sample.
/// @param volume
///     The channel volume.  0 to 127 (min to max)
/// @param pan
///     The channel pan 0 to 127 (left to right with 64 being centered).
///
/// @return
///     An integer id coresponding to the channel of playback. This value can be
///     used to pause, resume, or kill the sound as well as adjust volume, pan,
///     and frequency.
static inline int soundPlayPSG(DutyCycle cycle, u16 freq, u8 volume, u8 pan)
{
    return soundPlayPSGChannel(-1, cycle, freq, volume, pan);
}

/// Plays white noise with the specified parameters.
///
/// @param channel
///     Channel to be used for this sound. If -1 is used, libnds will select a
///     free channel and use it.
/// @param freq
///     The frequency in Hz of the sample.
/// @param volume
///     The channel volume. 0 to 127 (min to max).
/// @param pan
///     The channel pan 0 to 127 (left to right with 64 being centered).
///
/// @return
///     An integer id coresponding to the channel of playback. This value can be
///     used to pause, resume, or kill the sound as well as adjust volume, pan,
///     and frequency.
int soundPlayNoiseChannel(int channel, u16 freq, u8 volume, u8 pan);

/// Plays white noise with the specified parameters.
///
/// libnds will select a free channel to play the sound.
///
/// @param freq
///     The frequency in Hz of the sample.
/// @param volume
///     The channel volume. 0 to 127 (min to max).
/// @param pan
///     The channel pan 0 to 127 (left to right with 64 being centered).
///
/// @return
///     An integer id coresponding to the channel of playback. This value can be
///     used to pause, resume, or kill the sound as well as adjust volume, pan,
///     and frequency.
static inline int soundPlayNoise(u16 freq, u8 volume, u8 pan)
{
    return soundPlayNoiseChannel(-1, freq, volume, pan);
}

/// Pause the sound specified by soundId.
///
/// @param soundId
///     The sound ID returned by play sound.
void soundPause(int soundId);

/// Sets the Wave Duty of a PSG sound.
///
/// @param soundId
///     The sound ID returned by play sound.
/// @param cycle
///     The DutyCycle of the sound wave.
void soundSetWaveDuty(int soundId, DutyCycle cycle);

/// Stops the sound specified by soundId and frees any resources allocated.
///
/// @param soundId
///     The sound ID returned by play sound.
void soundKill(int soundId);

/// Resumes a paused sound.
///
/// @param soundId
///     The sound ID returned by play sound.
void soundResume(int soundId);

/// Sets the sound volume.
///
/// @param soundId
///     The sound ID returned by play sound.
/// @param volume
///     The new volume (0 to 127 min to max).
void soundSetVolume(int soundId, u8 volume);

/// Sets the sound panning.
//
/// @param soundId
///     The sound ID returned by play sound.
/// @param pan
///     The new pan value (0 to 127 left to right, 64 = center).
void soundSetPan(int soundId, u8 pan);

/// Sets the sound frequency.
///
/// @param soundId
///     The sound ID returned by play sound.
/// @param freq
///     The frequency in Hz.
void soundSetFreq(int soundId, u16 freq);

/// This starts a sound capture channel.
///
/// Audio capture channel 0 requires the frequency of sound channel 1 to be set
/// with soundSetFreq(), for example. For audio capture channel 1, you need to
/// do the same to sound channel 3. The sample rate of the capture circuit
/// matches the one of its corresponding sound channel because the channels are
/// the ones that can output the captured audio.
///
/// @param buffer
///     Buffer to store the captured audio.
/// @param bufferLen
///     Size of the buffer in words. A value of 0 will be treated as 1.
/// @param sndcapChannel
///     The audio capture channel to use.
/// @param addCapToChannel
///     For audio capture channel 0:
///     - If false, nothing special is done with the output of sound channel 1.
///     - If true, the output of sound channel 1 is added to sound channel 0.
///     For audio capture channel 1:
///     - If false, nothing special is done with the output of sound channel 3.
///     - If true, the output of sound channel 3 is added to sound channel 2.
/// @param sourceIsMixer
///     For audio capture channel 0:
///     - If true, the capture source is the output of the left mixer.
///     - If false, the capture source is sound channel 0.
///     For audio capture channel 1:
///     - If true, the capture source is the output of the right mixer.
///     - If false, the capture source is sound channel 2.
/// @param repeat
///     If true, the capture will continue even after the buffer is full (it
///     will go back to the start).
/// @param format
///     The audio format that will be used to store data in the provided buffer.
///
/// @returns
///     It returns the capture channel index on success, -1 on error.
int soundCaptureStart(void *buffer, u16 bufferLen, int sndcapChannel,
                      bool addCapToChannel, bool sourceIsMixer, bool repeat,
                      SoundCaptureFormat format);

/// This stops a sound capture channel.
///
/// @param sndcapChannel
///     The channel to stop.
void soundCaptureStop(int sndcapChannel);

/// Starts a microphone recording to a double buffer specified by buffer.
///
/// Note: The microphone uses timer 1 in the ARM7.
///
/// @param buffer
///     A pointer to the start of the double buffer.
/// @param bufferLength
///     The length of the buffer in bytes (both halfs of the double buffer).
/// @param format
///     Microphone can record in 8 or 12 bit format. 12 bit is shifted up to 16
///     bit PCM.
/// @param freq
///     The sample frequency.
/// @param callback
///     Called every time the buffer is full or half full.
///
/// @return
///     Returns non zero for success.
int soundMicRecord(void *buffer, u32 bufferLength, MicFormat format, int freq,
                   MicCallback callback);

/// Stops the microphone from recording
void soundMicOff(void);

/// Set extended sound hardware frequency.
///
/// @param freq_khz
///     Frequency in KHz. The default is 32, but 47 is allowed too.
void soundExtSetFrequency(unsigned int freq_khz);

/// Set the DSP/ARM volume ratio of the speakers output.
///
/// @param ratio
///     Value from 0 to 8 where 0 is 100% DSP, 8 is 100% ARM, 4 is 50% DSP and
///     50% ARM.
void soundExtSetRatio(unsigned int ratio);

/// Powers on the microphone so that it can be used by the DSP.
///
/// It won't record anything, it will simply remain powered on. You can power it
/// off with soundMicPowerOff().
///
/// This isn't required for soundMicRecord(), this is only required for the DSP.
/// Don't use it for anything else
void soundMicPowerOn(void);

/// Powers off the microphone after powering it on with soundMicPowerOn().
void soundMicPowerOff(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_SOUND_H__
