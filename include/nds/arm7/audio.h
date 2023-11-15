// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (c) 2023 Antonio Niño Díaz

// ARM7 audio control

#ifndef LIBNDS_NDS_ARM7_AUDIO_H__
#define LIBNDS_NDS_ARM7_AUDIO_H__

/// @file nds/arm7/audio.h
///
/// @brief Functions to use the audio channels and microphone from the ARM7.

#ifndef ARM7
#error Audio is only available on the ARM7
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/arm7/serial.h>
#include <nds/system.h>

#define SOUND_VOL(n)        (n)
#define SOUND_FREQ(n)       ((-0x1000000 / (n)))
#define SOUND_ENABLE        BIT(15)

#define SOUND_REPEAT        BIT(27)
#define SOUND_ONE_SHOT      BIT(28)

#define SOUND_FORMAT_16BIT  (1 << 29)
#define SOUND_FORMAT_8BIT   (0 << 29)
#define SOUND_FORMAT_PSG    (3 << 29)
#define SOUND_FORMAT_ADPCM  (2 << 29)

#define SOUND_PAN(n)        ((n) << 16)

#define SCHANNEL_ENABLE     BIT(31)

// Registers

#define REG_MASTER_VOLUME           (*(vu8 *)0x4000500)
#define REG_SOUNDCNT                (*(vu16 *)0x4000500)
#define REG_SOUNDBIAS               (*(vu32 *)0x4000504)

#define SCHANNEL_CR(n)              (*(vu32 *)(0x04000400 + ((n) << 4)))
#define SCHANNEL_VOL(n)             (*(vu8 *)(0x04000400 + ((n) << 4)))
#define SCHANNEL_PAN(n)             (*(vu8 *)(0x04000402 + ((n) << 4)))
#define SCHANNEL_SOURCE(n)          (*(vu32 *)(0x04000404 + ((n) << 4)))
#define SCHANNEL_TIMER(n)           (*(vu16 *)(0x04000408 + ((n) << 4)))
#define SCHANNEL_REPEAT_POINT(n)    (*(vu16 *)(0x0400040A + ((n) << 4)))
#define SCHANNEL_LENGTH(n)          (*(vu32 *)(0x0400040C + ((n) << 4)))

// Sound Capture Registers

#define REG_SNDCAP0CNT      (*(vu8 *)0x04000508)
#define REG_SNDCAP1CNT      (*(vu8 *)0x04000509)

#define REG_SNDCAP0DAD      (*(vu32 *)0x04000510)
#define REG_SNDCAP0LEN      (*(vu16 *)0x04000514)
#define REG_SNDCAP1DAD      (*(vu32 *)0x04000518)
#define REG_SNDCAP1LEN      (*(vu16 *)0x0400051C)

typedef void (*MIC_BUF_SWAP_CB)(u8 *completedBuffer, int length);

// DSi Registers

#define REG_SNDEXTCNT           (*(vu16 *)0x04004700)
#define REG_MICCNT              (*(vu16 *)0x04004600)
#define REG_MICDATA             (*(vu32 *)0x04004604)

#define SNDEXTCNT_RATIO(n)      ((n) & 0xF)
#define SNDEXTCNT_FREQ_32KHZ    (0 << 13) // Output freq 32.73kHz
#define SNDEXTCNT_FREQ_47KHZ    (1 << 13) // Output freq 47.61kHz
#define SNDEXTCNT_MUTE          BIT(14)
#define SNDEXTCNT_ENABLE        BIT(15)

#define MICCNT_FORMAT(n)        ((n) & 3) // Unknown, always set to '2'
#define MICCNT_FORMAT_MASK      (3)
// F / (n + 1) where F is SNDEXTCNT output freq
#define MICCNT_FREQ_DIV(n)      (((n) & 3) << 2)
#define MICCNT_FREQ_DIV_MASK    (3 << 2)
#define MICCNT_EMPTY            BIT(8)
#define MICCNT_NOT_EMPTY        BIT(9)
#define MICCNT_MORE_DATA        BIT(10)
#define MICCNT_OVERRUN          BIT(11)
#define MICCNT_CLEAR_FIFO       BIT(12)
#define MICCNT_ENABLE_IRQ       BIT(13)
#define MICCNT_ENABLE_IRQ2      BIT(14)
#define MICCNT_ENABLE           BIT(15)

/// Read a 8-bit unsigned value from the microphone.
///
/// @return The 8 bit unsigned value.
u8 micReadData8(void);

/// Read a 12-bit unsigned value from the microphone.
///
/// @return The 12 bit value.
u16 micReadData12(void);

/// Read a 16-bit signed value from the microphone.
///
/// @return The 16 bit signed value.
s16 micReadData16(void);

/// Start recording data from the microphone.
///
/// Fills the buffer with data from the microphone. The buffer will be signed
/// sound data at 16 kHz. Once the length of the buffer is reached, no more
/// data will be stored. It uses the specified ARM7 timer.
///
/// @param buffer Destination buffer.
/// @param length Destination buffer length in bytes.
/// @param freq Frequency of the recording.
/// @param timer Hardware timer to use to get samples from the microphone.
/// @param eightBitSample Set to true to record 8 bit samples instead of 12 bit.
/// @param bufferSwapCallback Callback called whenver the buffer is filled.
void micStartRecording(u8 *buffer, int length, int freq, int timer,
                       bool eightBitSample, MIC_BUF_SWAP_CB bufferSwapCallback);

/// Stop recording data, and return the length of data recorded.
///
/// @return Returns the length in bytes.
int micStopRecording(void);

/// Routine that must be called from a timer interrupt to get samples from the
/// microphone.
void micTimerHandler(void);

/// Turn amplifier on or off and set the gain in db.
///
/// @param control Use PM_AMP_ON or PM_AMP_OFF.
/// @param gain Use one of PM_GAIN_20, PM_GAIN_40, PM_GAIN_80 or PM_GAIN_160.
void micSetAmp(u8 control, u8 gain);

/// Turn the microphone ON.
static inline void micOn(void)
{
    micSetAmp(PM_AMP_ON, PM_GAIN_160);
}

/// Turn the microphone OFF.
static inline void micOff(void)
{
    micSetAmp(PM_AMP_OFF, 0);
}

/// Enable sound hardware and clear sound registers.
void enableSound(void);

/// Disable sound hardware.
void disableSound(void);

/// Install the libnds sound FIFO handler.
void installSoundFIFO(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_AUDIO_H__
