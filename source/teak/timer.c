// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#include <teak/timer.h>

void timerStart(u16 index, u16 config, u32 reload_value)
{
    // Stop timer
    REG_TMR_CONTROL(index) = TMR_CONTROL_PAUSE;

    // This value is used as starting value. In mode reload, it is also used as
    // reload value. It is ignored in freerun mode.
    REG_TMR_RELOAD_LO(index) = reload_value & 0xFFFF;
    REG_TMR_RELOAD_HI(index) = reload_value >> 16;

    REG_TMR_CONTROL(index) = config | TMR_CONTROL_RESTART;
}

u32 timerRead(u16 index)
{
    u16 old = REG_TMR_CONTROL(index);

    // Freeze counter. This doesn't stop the counter, it just freezes the value
    // present in the counter registers. This is required to avoid race
    // conditions when reading from the counter registers.
    REG_TMR_CONTROL(index) = old & ~TMR_CONTROL_FREEZE_MASK;

    u32 value = REG_TMR_COUNTER_LO(index);
    value |= ((u32)REG_TMR_COUNTER_HI(index)) << 16;

    REG_TMR_CONTROL(index) = old;

    return value;
}
