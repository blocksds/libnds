// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

/// @file nds/system_counter.h
///
/// @brief Contains a system tick counter that can be used by other parts of
/// libnds and other libraries and user code.

#ifndef LIBNDS_NDS_SYSTEM_COUNTER_H__
#define LIBNDS_NDS_SYSTEM_COUNTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>
#include <nds/timers.h>

// The bus frequency is 33.513982 MHz (a resolution of around 30 ns). A timer
// with a divider of 1 would trigger interrupts 8 times per frame, which is
// quite costly.
//
// The system tick counter uses a divider of 64, so the timer frequency is
// 523.656 KHz (a resolution of around 2 us). This way there is only an
// interrupt every 8 frames.

/// This gets the current value of the system tick counter.
///
/// @return
///     A 64-bit value with the number of ticks.
uint64_t systemCounterGetTicks(void);

/// Initializes the system tick counter using a timer.
///
/// It uses the timer with index LIBNDS_TIMER_TICK_COUNTER.
void systemCounterSetup(void);

/// Converts system counter ticks to microseconds.
///
/// @param ticks
///     Number of ticks.
/// @return
///     Number of microseconds.
static inline u32 systemCounterTicksToUsec(u32 ticks)
{
    return (((u64)ticks) * 1000000) / (BUS_CLOCK / 64);
}

/// Converts system counter ticks to milliseconds.
///
/// @param ticks
///     Number of ticks.
/// @return
///     Number of milliseconds.
static inline u32 systemCounterTicksToMsec(u32 ticks)
{
    return (((u64)ticks) * 1000) / (BUS_CLOCK / 64);
}

/// Converts microseconds to system counter ticks.
///
/// @param us
///     Number of microseconds.
/// @return
///     Number of ticks.
static inline u32 systemCounterUsecsToTicks(u32 us)
{
    return ((u64)us * (BUS_CLOCK / 64)) / 1000000;
}

/// Converts milliseconds to system counter ticks.
///
/// @param ms
///     Number of milliseconds.
/// @return
///     Number of ticks.
static inline u32 systemCounterMsecsToTicks(u32 ms)
{
    return ((u64)ms * (BUS_CLOCK / 64)) / 1000;
}

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_SYSTEM_COUNTER_H__
