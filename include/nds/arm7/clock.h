// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_ARM7_CLOCK_H__
#define LIBNDS_NDS_ARM7_CLOCK_H__

/// @file nds/arm7/clock.h
///
/// @brief Utilities to read and write the real time clock from the ARM7.

#ifndef ARM7
#error The clock is only available on the ARM7
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/arm7/serial.h>
#include <nds/system.h>

// RTC registers
#define WRITE_STATUS_REG1   0x60
#define READ_STATUS_REG1    0x61

// Status Register 1
//  0   W   Reset                (0=Normal, 1=Reset)
//  1   R/W 12/24 hour mode      (0=12 hour, 1=24 hour)
//  2-3 R/W General purpose bits
//  4   R   Interrupt 1 Flag (1=Yes)                      ;auto-cleared on read
//  5   R   Interrupt 2 Flag (1=Yes)                      ;auto-cleared on read
//  6   R   Power Low Flag (0=Normal, 1=Power is/was low) ;auto-cleared on read
//  7   R   Power Off Flag (0=Normal, 1=Power was off)    ;auto-cleared on read
//  Power off indicates that the battery was removed or fully discharged,
//  all registers are reset to 00h (or 01h), and must be re-initialized.

// Read-only, cleared by reading (1 if just powered on)
#define STATUS_POC      (1 << 7)
// Read-only, cleared by reading (1 if power dropped below the safety threshold)
#define STATUS_BLD      (1 << 6)
#define STATUS_INT2     (1 << 5)  // Read-only, INT2 has occured
#define STATUS_INT1     (1 << 4)  // Read-only, INT1 has occured
#define STATUS_SC1      (1 << 3)  // R/W scratch bit
#define STATUS_SC0      (1 << 2)  // R/W scratch bit
#define STATUS_24HRS    (1 << 1)  // 24 hour mode when 1, 12 hour mode when 0
#define STATUS_RESET    (1 << 0)  // write-only, reset when 1 written

#define WRITE_STATUS_REG2   0x62
#define READ_STATUS_REG2    0x63

// Status Register 2
//   0-3 R/W INT1 Mode/Enable
//         0000b Disable
//         0x01b Selected Frequency steady interrupt
//         0x10b Per-minute edge interrupt
//         0011b Per-minute steady interrupt 1 (duty 30.0 secomds)
//         0100b Alarm 1 interrupt
//         0111b Per-minute steady interrupt 2 (duty 0.0079 secomds)
//         1xxxb 32kHz output
// 4-5 R/W General purpose bits
// 6   R/W INT2 Enable
//         0b    Disable
//         1b    Alarm 2 interrupt
// 7   R/W Test Mode (0=Normal, 1=Test, don't use) (cleared on Reset)
#define STATUS_TEST         (1 << 7)
#define STATUS_INT2AE       (1 << 6)
#define STATUS_SC3          (1 << 5)  // R/W scratch bit
#define STATUS_SC2          (1 << 4)  // R/W scratch bit

#define STATUS_32kE         (1 << 3) // Interrupt mode bits
#define STATUS_INT1AE       (1 << 2)
#define STATUS_INT1ME       (1 << 1)
#define STATUS_INT1FE       (1 << 0)

// full 7 bytes for time and date
#define WRITE_TIME_AND_DATE     0x64
#define READ_TIME_AND_DATE      0x65

// Last 3 bytes of current time
#define WRITE_TIME              0x66
#define READ_TIME               0x67

#define WRITE_INT_REG1          0x68
#define READ_INT_REG1           0x69

#define WRITE_INT_REG2          0x6A
#define READ_INT_REG2           0x6B

#define READ_CLOCK_ADJUST_REG   0x6C
#define WRITE_CLOCK_ADJUST_REG  0x6D
// Clock-adjustment register

#define READ_FREE_REG           0x6E
#define WRITE_FREE_REG          0x6F


void rtcReset(void);
void rtcTransaction(uint8_t *command, uint32_t commandLength, uint8_t *result,
                    uint32_t resultLength);

// All of the deprecated helpers are using byte arrays as input/output types
// instead of structures.

__attribute__((deprecated)) void rtcGetTime(uint8_t *time);
__attribute__((deprecated)) void rtcSetTime(uint8_t *time);

__attribute__((deprecated)) void rtcGetTimeAndDate(uint8_t *time);
__attribute__((deprecated)) void rtcSetTimeAndDate(uint8_t *time);

void BCDToInteger(uint8_t *data, uint32_t length);
void integerToBCD(uint8_t *data, uint32_t length);

/// Initialize the RTC and setup the RTC interrupt to update the time.
///
/// @deprecated The RTC interrupt isn't supported by the 3DS in DS or DSi modes.
/// Any application that uses it will run on DS and DSi consoles, but not on
/// 3DS. Also, most emulators don't support it either. The alternative is to use
/// initClockIRQTimer(), which uses a timer interrupt instead.
__attribute__((deprecated)) void initClockIRQ(void);

/// Initialize the RTC and setup a timer interrupt to update the time.
///
/// @param timer Timer index to use.
void initClockIRQTimer(int timer);

/// Fills the provided rtcTime structure with the current time.
///
/// @param rtc Pointer to the rtcTime struct to fill.
void rtcTimeGet(rtcTime *rtc);

/// Sets the current time to the provided rtcTime structure.
///
/// A returned value of 0 doesn't mean that the RTC registers were updated
/// correctly, only that the checks previous to writing the RTC registers
/// passed.
///
/// @param rtc Pointer to the rtcTime struct with the new time.
/// @return If the provided values were valid, it returns 0, else -1.
int rtcTimeSet(rtcTime *rtc);

/// Fills the provided rtcTimeAndDate structure with the current time and date.
///
/// @param rtc Pointer to the rtcTimeAndDate struct to fill.
void rtcTimeAndDateGet(rtcTimeAndDate *rtc);

/// Saves the current time and date to the provided rtcTimeAndDate structure.
///
/// A returned value of 0 doesn't mean that the RTC registers were updated
/// correctly, only that the checks previous to writing the RTC registers
/// passed.
///
/// @param rtc Pointer to the rtcTimeAndDate struct with the new time and date.
/// @return If the provided values were valid, it returns 0, else -1.
int rtcTimeAndDateSet(rtcTimeAndDate *rtc);

/// Reads RTC registers and updates the internal time of libnds.
void resyncClock(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_CLOCK_H__
