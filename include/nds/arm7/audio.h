// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2023-2025 Antonio Niño Díaz

// ARM7 audio control

#ifndef LIBNDS_NDS_ARM7_AUDIO_H__
#define LIBNDS_NDS_ARM7_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/arm7/audio.h
///
/// @brief Functions to use the audio channels and microphone from the ARM7.

#ifndef ARM7
#error Audio is only available on the ARM7
#endif

#include <nds/arm7/serial.h>
#include <nds/system.h>
#include <nds/timers.h>

// Control registers
// -----------------

#define REG_SOUNDCNT                (*(vu16 *)0x4000500)

#define SOUNDCNT_VOL(n)             (n) // 0 (mute) to 127 (max)
#define SOUNDCNT_ENABLE             BIT(15)

#define REG_MASTER_VOLUME           (*(vu8 *)0x4000500) // Low byte of REG_SOUNDCNT

#define REG_SOUNDBIAS               (*(vu32 *)0x4000504)

// Sound channel registers
// -----------------------

#define REG_SOUNDXCNT(n)            (*(vu32 *)(0x04000400 + ((n) << 4)))

#define SOUNDXCNT_VOL_MUL(v)        (v) // 0 (mute) to 127 (max)

#define SOUNDXCNT_VOL_DIV(v)        ((v) << 8)

#define SOUNDXCNT_VOL_DIV_1         SCHANNEL_VOL_DIV(0)
#define SOUNDXCNT_VOL_DIV_2         SCHANNEL_VOL_DIV(1)
#define SOUNDXCNT_VOL_DIV_4         SCHANNEL_VOL_DIV(2)
#define SOUNDXCNT_VOL_DIV_16        SCHANNEL_VOL_DIV(3)

#define SOUNDXCNT_PAN(n)            ((n) << 16) // 0 (left) to 64 (center) to 127 (right)

#define SOUNDXCNT_DONT_HOLD         0
#define SOUNDXCNT_HOLD              BIT(15) // Hold last sample of one-shot sound

#define SOUNDXCNT_DUTY(v)           ((v) << 24) // HIGH % = (v + 1) * 12.5% (PSG only)

#define SOUNDXCNT_MANUAL            0
#define SOUNDXCNT_REPEAT            BIT(27)
#define SOUNDXCNT_ONE_SHOT          BIT(28)

#define SOUNDXCNT_FORMAT_16BIT      (1 << 29)
#define SOUNDXCNT_FORMAT_8BIT       (0 << 29)
#define SOUNDXCNT_FORMAT_PSG        (3 << 29)
#define SOUNDXCNT_FORMAT_ADPCM      (2 << 29)

#define SOUNDXCNT_ENABLE            BIT(31)

// Parts of REG_SOUNDXCNT(n)
#define REG_SOUNDXVOL(n)            (*(vu8 *)(0x04000400 + ((n) << 4)))
#define REG_SOUNDXPAN(n)            (*(vu8 *)(0x04000402 + ((n) << 4)))

#define REG_SOUNDXSAD(n)            (*(vu32 *)(0x04000404 + ((n) << 4)))

#define REG_SOUNDXTMR(n)            (*(vu16 *)(0x04000408 + ((n) << 4)))

#define SOUNDXTMR_FREQ(n)            TIMER_FREQ_SHIFT(n, 1, 1)

#define REG_SOUNDXPNT(n)            (*(vu16 *)(0x0400040A + ((n) << 4)))

#define REG_SOUNDXLEN(n)            (*(vu32 *)(0x0400040C + ((n) << 4)))

// Old names
#define SOUND_PAN(n)                SOUNDXCNT_PAN(n)
#define SOUND_MANUAL                SOUNDXCNT_MANUAL
#define SOUND_REPEAT                SOUNDXCNT_REPEAT
#define SOUND_ONE_SHOT              SOUNDXCNT_ONE_SHOT
#define SOUND_FORMAT_16BIT          SOUNDXCNT_FORMAT_16BIT
#define SOUND_FORMAT_8BIT           SOUNDXCNT_FORMAT_8BIT
#define SOUND_FORMAT_PSG            SOUNDXCNT_FORMAT_PSG
#define SOUND_FORMAT_ADPCM          SOUNDXCNT_FORMAT_ADPCM
#define SCHANNEL_ENABLE             SOUNDXCNT_ENABLE
#define SOUND_FREQ(n)               SOUNDXTMR_FREQ(n)
#define SCHANNEL_CR(n)              REG_SOUNDXCNT(n)
#define SCHANNEL_VOL(n)             REG_SOUNDXVOL(n)
#define SCHANNEL_PAN(n)             REG_SOUNDXPAN(n)
#define SCHANNEL_SOURCE(n)          REG_SOUNDXSAD(n)
#define SCHANNEL_TIMER(n)           REG_SOUNDXTMR(n)
#define SCHANNEL_REPEAT_POINT(n)    REG_SOUNDXPNT(n)
#define SCHANNEL_LENGTH(n)          REG_SOUNDXLEN(n)
#define SOUND_VOL(n)                SOUNDCNT_VOL(n)
#define SOUND_ENABLE                SOUNDCNT_ENABLE

// Sound Capture Registers
// -----------------------

#define REG_SNDCAP0CNT      (*(vu8 *)0x04000508)
#define REG_SNDCAP1CNT      (*(vu8 *)0x04000509)

#define SND0CAPCNT_CH1_OUT_DIRECT       (0 << 0)
#define SND0CAPCNT_CH1_OUT_ADD_TO_CH0   (1 << 0)
#define SND0CAPCNT_SOURCE_LEFT_MIXER    (0 << 1)
#define SND0CAPCNT_SOURCE_CH0           (1 << 1)

#define SND1CAPCNT_CH3_OUT_DIRECT       (0 << 0)
#define SND1CAPCNT_CH3_OUT_ADD_TO_CH2   (1 << 0)
#define SND1CAPCNT_SOURCE_RIGHT_MIXER   (0 << 1)
#define SND1CAPCNT_SOURCE_CH2           (1 << 1)

#define SNDCAPCNT_REPEAT                (0 << 2)
#define SNDCAPCNT_ONESHOT               (1 << 2)
#define SNDCAPCNT_FORMAT_16BIT          (0 << 3)
#define SNDCAPCNT_FORMAT_8BIT           (1 << 3)
#define SNDCAPCNT_STOP                  (0 << 7)
#define SNDCAPCNT_START_BUSY            (1 << 7)

// The destination must be word-aligned, and the sizes are in words
#define REG_SNDCAP0DAD      (*(vu32 *)0x04000510)
#define REG_SNDCAP0LEN      (*(vu16 *)0x04000514)
#define REG_SNDCAP1DAD      (*(vu32 *)0x04000518)
#define REG_SNDCAP1LEN      (*(vu16 *)0x0400051C)

/// Callback called whenever one buffer used micStartRecording() is full.
///
/// @param completedBuffer
///     Pointer to the buffer that has been filled.
/// @param length
///     Size of the buffer in bytes.
typedef void (*MIC_BUF_SWAP_CB)(u8 *completedBuffer, int length);

// DSi Registers
// -------------

#define REG_SNDEXTCNT           (*(vu16 *)0x04004700)

#define SNDEXTCNT_RATIO(n)      ((n) & 0xF)
#define SNDEXTCNT_FREQ_32KHZ    (0 << 13) // Output freq 32.73kHz
#define SNDEXTCNT_FREQ_47KHZ    (1 << 13) // Output freq 47.61kHz
#define SNDEXTCNT_MUTE          BIT(14)
#define SNDEXTCNT_ENABLE        BIT(15)

#define REG_MICCNT              (*(vu16 *)0x04004600)

#define MICCNT_FORMAT(n)        ((n) & 3)
#define MICCNT_FORMAT_MASK      (3)
#define MICCNT_FORMAT_STEREO    MICCNT_FORMAT(0) // Repeat every sample twice
#define MICCNT_FORMAT_NORMAL    MICCNT_FORMAT(2)

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

#define REG_MICDATA             (*(vu32 *)0x04004604)

/// Read a 8-bit unsigned value from the microphone.
///
/// @return
///     The 8 bit unsigned value.
u8 micReadData8(void);

/// Read a 12-bit unsigned value from the microphone.
///
/// @return
///     The 12 bit value.
u16 micReadData12(void);

/// Read a 16-bit signed value from the microphone.
///
/// @return
///     The 16 bit signed value.
s16 micReadData16(void);

/// Start recording data from the microphone.
///
/// Fills the buffer with data from the microphone. The buffer will be signed
/// sound data at 16 kHz. Once the length of the buffer is reached, no more
/// data will be stored. It uses the specified ARM7 timer.
///
/// @param buffer
///     Destination buffer.
/// @param length
///     Destination buffer length in bytes.
/// @param freq
///     Frequency of the recording.
/// @param timer
///     Hardware timer to use to get samples from the microphone.
/// @param eightBitSample
///     Set to true to record 8 bit samples instead of 12 bit.
/// @param bufferSwapCallback
///     Callback called whenver the buffer is filled.
void micStartRecording(u8 *buffer, int length, int freq, int timer,
                       bool eightBitSample, MIC_BUF_SWAP_CB bufferSwapCallback);

/// Stop recording data, and return the length of data recorded.
///
/// @return
///     Returns the length in bytes.
int micStopRecording(void);

/// Routine that must be called from a timer interrupt to get samples from the
/// microphone.
void micTimerHandler(void);

/// Turn amplifier on or off and set the gain in db.
///
/// @param control
///     Use PM_AMP_ON or PM_AMP_OFF.
/// @param gain
///     Use one of PM_GAIN_20, PM_GAIN_40, PM_GAIN_80 or PM_GAIN_160.
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

/// Set extended sound hardware frequency.
///
/// @param freq_khz
///     Frequency in KHz. The default is 32, but 47 is allowed too.
///
/// @return
///     Returns true if the change was successful.
bool soundExtSetFrequencyTWL(unsigned int freq_khz);

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
