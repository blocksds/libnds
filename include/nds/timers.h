// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/timers.h
///
/// @brief Contains defines, macros and functions for ARM7 and ARM9 timer
/// operation.
///
/// It also contains a simplified API for timer use and some cpu timing functions.
///
/// The timers are fed with a 33.513982 MHz source on the ARM9 and ARM7.
///
/// @note that dswifi will use timer 3 on the arm9, so don't use that if you use dswifi.

#ifndef LIBNDS_NDS_TIMERS_H__
#define LIBNDS_NDS_TIMERS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

/// Returns a dereferenced pointer to the data register for timer control
/// register.
///
/// **Example Usage:**
/// ```
/// TIMER_CR(x) = TIMER_ENABLE | ClockDivider_64;
/// ```
///
/// Possible bit defines:
///
/// @see TIMER_ENABLE
/// @see TIMER_IRQ_REQ
/// @see TIMER_CASCADE
/// @see ClockDivider
#define TIMER_CR(n) (*(vu16 *)(0x04000102 + ((n) << 2)))

/// Same as TIMER_CR(0).
#define TIMER0_CR   (*(vu16 *)0x04000102)
/// Same as TIMER_CR(1).
#define TIMER1_CR   (*(vu16 *)0x04000106)
/// Same as TIMER_CR(2).
#define TIMER2_CR   (*(vu16 *)0x0400010A)
/// Same as TIMER_CR(3).
#define TIMER3_CR   (*(vu16 *)0x0400010E)

/// Returns a dereferenced pointer to the data register for timer number "n".
///
/// @see TIMER_CR(n)
/// @see TIMER_FREQ(n)
///
/// TIMER_DATA(n) when set will latch that value into the counter. Every time
/// the counter rolls over, TIMER_DATA(0) will return to the latched value.
/// This allows you to control the frequency of the timer using the following
/// formula:
/// ```
/// TIMER_DATA(x) = -(BUS_CLOCK / (freq * divider));
/// ```
///
/// **Example Usage:**
/// ```
/// TIMER_DATA(0) = value; // 0 to 3. value is 16 bits
/// ```
#define TIMER_DATA(n)  (*(vu16 *)(0x04000100 + ((n) << 2)))

/// Same as TIMER_DATA(0).
#define TIMER0_DATA    (*(vu16 *)0x04000100)
/// Same as TIMER_DATA(1).
#define TIMER1_DATA    (*(vu16 *)0x04000104)
/// Same as TIMER_DATA(2).
#define TIMER2_DATA    (*(vu16 *)0x04000108)
/// Same as TIMER_DATA(3).
#define TIMER3_DATA    (*(vu16 *)0x0400010C)

/// The speed in which the timer ticks in Hz.
#define BUS_CLOCK       (33513982)

/// Enables the timer.
#define TIMER_ENABLE    (1 << 7)
/// Causes the timer to request an Interrupt on overflow.
#define TIMER_IRQ_REQ   (1 << 6)
/// When set will cause the timer to count when the timer below overflows
/// (unavailable for timer 0).
#define TIMER_CASCADE   (1 << 2)

/// Allowable timer clock dividers.
typedef enum {
    ClockDivider_1 = 0,     ///< Divides the timer clock by 1 (~33513.982 kHz)
    ClockDivider_64 = 1,    ///< Divides the timer clock by 64 (~523.657 kHz)
    ClockDivider_256 = 2,   ///< Divides the timer clock by 256 (~130.914 kHz)
    ClockDivider_1024 = 3   ///< divides the timer clock by 1024 (~32.7284 kHz)
} ClockDivider;

// Use the ClockDivider enum instead of the defines below

/// Causes the timer to count at 33.514 MHz.
#define TIMER_DIV_1     (0)
/// Causes the timer to count at (33.514 / 64) MHz.
#define TIMER_DIV_64    (1)
/// Causes the timer to count at (33.514 / 256) MHz.
#define TIMER_DIV_256   (2)
/// Causes the timer to count at (33.514 / 1024) MHz.
#define TIMER_DIV_1024  (3)

/// A macro that calculates TIMER_DATA(n) settings for a given frequency of n.
///
/// It will calculate the correct value for TIMER_DATA(n) given the frequency
/// in Hz (number of times the timer should overflow per second).
///
/// **Example Usage:**
/// ```
/// // Calls the timerCallBack function 5 times per second.
/// timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(5), timerCallBack);
/// ```
///
/// Max frequency is: 33554432 Hz
/// Min frequency is: 512 Hz
///
/// @note Use the appropriate macro depending on the used clock divider.
#define TIMER_FREQ(n)    (-BUS_CLOCK / (n))

/// A macro that calculates TIMER_DATA(n) settings for a given frequency of n.
///
/// It will calculate the correct value for TIMER_DATA(n) given the frequency
/// in Hz (number of times the timer should overflow per second).
///
/// **Example Usage:**
/// ```
/// // Calls the timerCallBack function 5 times per second.
/// timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(5), timerCallBack);
/// ```
///
/// Max frequency is: 524288 Hz
/// Min frequency is: 8 Hz
///
/// @note Use the appropriate macro depending on the used clock divider.
#define TIMER_FREQ_64(n) (-(BUS_CLOCK >> 6) / (n))

/// A macro that calculates TIMER_DATA(n) settings for a given frequency of n.
///
/// It will calculate the correct value for TIMER_DATA(n) given the frequency
/// in Hz (number of times the timer should overflow per second).
///
/// **Example Usage:**
/// ```
/// // Calls the timerCallBack function 5 times per second.
/// timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(5), timerCallBack);
/// ```
///
/// Max frequency is: 131072 Hz
/// Min frequency is: 2 Hz
///
/// @note Use the appropriate macro depending on the used clock divider.
#define TIMER_FREQ_256(n) (-(BUS_CLOCK >> 8) / (n))

/// Macro that calculates TIMER_DATA(n) settings for a given frequency of n.
///
/// It wtill calculate the correct value for TIMER_DATA(n) given the frequency
/// in Hz (number of times the timer should overflow per second).
///
/// **Example Usage:**
/// ```
/// // Calls the timerCallBack function 5 times per second.
/// timerStart(0, ClockDivider_1024, TIMER_FREQ_1024(5), timerCallBack);
/// ```
///
/// Max frequency is: 32768 Hz
/// Min frequency is: 0.5 Hz
///
/// @note Use the appropriate macro depending on the used clock divider.
#define TIMER_FREQ_1024(n) (-(BUS_CLOCK >> 10) / (n))

/// Start a hardware timer.
///
/// Callback is tied directly to interrupt table and called directly, resulting
/// in less latency than the attached timer.
///
/// @param timer The hardware timer to use (0 - 3).
/// @param divider The timer channel clock divider (clock will tick at 33.513982
///                MHz / divider).
/// @param ticks The number of ticks which must elapse before the timer
///              overflows.
/// @param callback The callback to be called when the timer expires (if NULL,
///                 no IRQ will be generated by the timer).
void timerStart(int timer, ClockDivider divider, u16 ticks, VoidFn callback);

/// Returns the ticks elapsed since the last call to timerElapsed().
///
/// @param timer The hardware timer to use (0 - 3).
/// @return The number of ticks which have elapsed since the last call to
///         timerElapsed().
u16 timerElapsed(int timer);

/// Returns the raw ticks of the specified timer.
///
/// @param timer The hardware timer to use (0 - 3).
/// @return the raw ticks of the specified timer data register.
static inline u16 timerTick(int timer)
{
    return TIMER_DATA(timer);
}

/// Pauses the specified timer.
///
/// @param timer The hardware timer to use (0 - 3).
/// @return The number of ticks which have elapsed since the last call to
///         timerElapsed().
u16 timerPause(int timer);

/// Unpauses the specified timer.
///
/// @param timer The hardware timer to use (0 - 3).
static inline void timerUnpause(int timer)
{
    TIMER_CR(timer) |= TIMER_ENABLE;
}

/// Stops the specified timer.
///
/// @param timer The hardware timer to use (0 - 3).
/// @return The number of ticks which have elapsed since the last call to
///         timerElapsed().
u16 timerStop(int timer);

/// Begins CPU timing using two timers for 32bit resolution.
///
/// @param timer The base hardware timer to use (0 - 2).
void cpuStartTiming(int timer);

/// Returns the number of ticks which have elapsed since cpuStartTiming.
///
/// @return The number of ticks which have elapsed since cpuStartTiming.
u32 cpuGetTiming(void);

/// Ends CPU timing.
///
/// @return The number of ticks which have elapsed since cpuStartTiming.
u32 cpuEndTiming(void);

static inline u32 timerTicks2usec(u32 ticks)
{
    return (((u64)ticks) * 1000000) / BUS_CLOCK;
}

static inline u32 timerTicks2msec(u32 ticks)
{
    return (((u64)ticks) * 1000) / BUS_CLOCK;
}

static inline u16 timerFreqToTicks_1(int freq)
{
    return -BUS_CLOCK / freq;
}

static inline u16 timerFreqToTicks_64(int freq)
{
    return (-BUS_CLOCK >> 6) / freq;
}

static inline u16 timerFreqToTicks_256(int freq)
{
    return (-BUS_CLOCK >> 8) / freq;
}

static inline u16 timerFreqToTicks_1024(int freq)
{
    return (-BUS_CLOCK >> 10) / freq;
}

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_TIMERS_H__
