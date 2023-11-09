// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2020 Gericom
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBTEAK_TIMER_H__
#define LIBTEAK_TIMER_H__

#include <teak/types.h>

#define TMR_REG_BASE                0x8020
#define TMR_CHANNEL_LEN             0x10

#define REG_TMR_CONTROL(x)          (*(vu16 *)(TMR_REG_BASE + (x) * TMR_CHANNEL_LEN + 0x00))

#define TMR_CONTROL_PRESCALE_1          (0 << 0)
#define TMR_CONTROL_PRESCALE_2          (1 << 0)
#define TMR_CONTROL_PRESCALE_4          (2 << 0)
#define TMR_CONTROL_PRESCALE_16         (3 << 0)

#define TMR_CONTROL_PRESCALE_MASK       (3 << 0)

#define TMR_CONTROL_MODE_ONCE           (0 << 2)
#define TMR_CONTROL_MODE_RELOAD         (1 << 2)
#define TMR_CONTROL_MODE_FREERUN        (2 << 2)
#define TMR_CONTROL_MODE_EVENTCNT       (3 << 2)
#define TMR_CONTROL_MODE_WATCHDOG_RESET (4 << 2)
#define TMR_CONTROL_MODE_WATCHDOG_NMI   (5 << 2)
#define TMR_CONTROL_MODE_WATCHDOG_IRQ   (6 << 2)

#define TMR_CONTROL_MODE_MASK           (7 << 2)

#define TMR_CONTROL_POLARITY_NORMAL     (0 << 6)
#define TMR_CONTROL_POLARITY_INVERT     (1 << 6)

#define TMR_CONTROL_CLEAR_OUTPUT        (1 << 7)

#define TMR_CONTROL_UNPAUSE             (0 << 8)
#define TMR_CONTROL_PAUSE               (1 << 8)

#define TMR_CONTROL_FREEZE_COUNTER      (0 << 9)
#define TMR_CONTROL_UNFREEZE_COUNTER    (1 << 9)
#define TMR_CONTROL_FREEZE_MASK         (1 << 9)

#define TMR_CONTROL_RESTART             (1 << 10)

#define TMR_CONTROL_BREAKPOINT          (1 << 11)

#define TMR_CONTROL_CLOCK_INTERNAL      (0 << 12)
#define TMR_CONTROL_CLOCK_EXTERNAL      (1 << 12) // Unused?

#define TMR_CONTROL_UNKNOWN             (1 << 13)

#define TMR_CONTROL_AUTOCLEAR_OFF       (0 << 14)
#define TMR_CONTROL_AUTOCLEAR_2_CYCLES  (1 << 14)
#define TMR_CONTROL_AUTOCLEAR_4_CYCLES  (2 << 14)
#define TMR_CONTROL_AUTOCLEAR_8_CYCLES  (3 << 14)

#define TMR_CONTROL_AUTOCLEAR_MASK      (3 << 14)

#define REG_TMR_TRIGGER(x)          (*(vu16 *)(TMR_REG_BASE + (x) * TMR_CHANNEL_LEN + 0x02))

// Start value and reload value. Set it before starting the timer.
#define REG_TMR_RELOAD_LO(x)        (*(vu16 *)(TMR_REG_BASE + (x) * TMR_CHANNEL_LEN + 0x04))
#define REG_TMR_RELOAD_HI(x)        (*(vu16 *)(TMR_REG_BASE + (x) * TMR_CHANNEL_LEN + 0x06))

// Current value of the timer.
#define REG_TMR_COUNTER_LO(x)       (*(vu16 *)(TMR_REG_BASE + (x) * TMR_CHANNEL_LEN + 0x08))
#define REG_TMR_COUNTER_HI(x)       (*(vu16 *)(TMR_REG_BASE + (x) * TMR_CHANNEL_LEN + 0x0A))

#define REG_TMR_PWM_RELOAD_LO(x)    (*(vu16 *)(TMR_REG_BASE + (x) * TMR_CHANNEL_LEN + 0x0C))
#define REG_TMR_PWM_RELOAD_HI(x)    (*(vu16 *)(TMR_REG_BASE + (x) * TMR_CHANNEL_LEN + 0x0E))

/// Starts a timer with the specified starting value.
///
/// @param index Timer index (0 or 1).
/// @param config Configuration flags.
/// @param reload_value Starting value. In mode reload, it is also used as
///                     reload value. It is ignored in freerun mode.
void timerStart(u16 index, u16 config, u32 reload_value);

/// Safely reads the 32-bit counter value of a timer.
///
/// @param index Timer index (0 or 1).
/// @return The counter value.
u32 timerRead(u16 index);

/// Stops a timer.
///
/// @param index Timer index (0 or 1).
static inline void timerStop(u16 index)
{
    REG_TMR_CONTROL(index) = TMR_CONTROL_PAUSE;
}

#endif // LIBTEAK_TIMER_H__
