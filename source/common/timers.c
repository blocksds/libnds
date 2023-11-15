// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008 Jason Rogers (dovoto)
// Copyright (C) 2009 Mukunda Johnson (eKid)
// Copyright (C) 2010-2011 Dave Murphy (WinterMute)

// Timer API

#include <nds/interrupts.h>
#include <nds/timers.h>

#ifdef ARM9
#include <nds/arm9/sassert.h>
#endif
#ifdef ARM7
#define sassert(v, s)
#endif

void timerStart(int timer, ClockDivider divider, u16 ticks, VoidFn callback)
{
    sassert(timer < 4, "timer must be in range 0 - 3");
    TIMER_DATA(timer) = ticks;

    if (callback)
    {
        irqSet(IRQ_TIMER(timer), callback);
        irqEnable(IRQ_TIMER(timer));
        TIMER_CR(timer) = TIMER_IRQ_REQ | divider | TIMER_ENABLE;
    }
    else
    {
        TIMER_CR(timer) = divider | TIMER_ENABLE;
    }
}

u16 elapsed[4] = {0, 0, 0, 0};

u16 timerElapsed(int timer)
{
    sassert(timer < 4, "timer must be in range 0 - 3");
    u16 time = TIMER_DATA(timer);

    s32 result = (s32)time - (s32)elapsed[timer];

    // Overflow. This will only be accurate if it has overflowed no more than once.
    if (result < 0)
        result = time + (0x10000 - elapsed[timer]);

    elapsed[timer] = time;

    return (u16) result;
}

u16 timerPause(int timer)
{
    sassert(timer < 4, "timer must be in range 0 - 3");
    TIMER_CR(timer) &= ~TIMER_ENABLE;
    u16 temp = timerElapsed(timer);
    elapsed[timer] = 0;
    return temp;
}

u16 timerStop(int timer)
{
    sassert(timer < 4, "timer must be in range 0 - 3");
    TIMER_CR(timer) = 0;
    u16 temp = timerElapsed(timer);
    elapsed[timer] = 0;
    return temp;
}

/*
  CPU Usage - http://forums.devkitpro.org/viewtopic.php?f=6&t=415

  original Source by eKid
  adapted by Ryouarashi and Weirdfox
*/
static int localTimer = 0;

void cpuStartTiming(int timer)
{
    sassert(timer < 3, "timer must be in range 0 - 2");
    localTimer = timer;

    TIMER_CR(timer) = 0;
    TIMER_CR(timer + 1) = 0;

    TIMER_DATA(timer) = 0;
    TIMER_DATA(timer + 1) = 0;

    TIMER_CR(timer + 1) = TIMER_CASCADE | TIMER_ENABLE;
    TIMER_CR(timer) = TIMER_ENABLE;
}

u32 cpuGetTiming(void)
{
    int lo = TIMER_DATA(localTimer);
    int hi = TIMER_DATA(localTimer + 1);
    int lo2 = TIMER_DATA(localTimer);
    int hi2 = TIMER_DATA(localTimer + 1);

    if (lo2 < lo)
    {
        lo = lo2;
        hi = hi2;
    }

    return lo | hi << 16;
}

u32 cpuEndTiming(void)
{
    int lo = TIMER_DATA(localTimer);
    int hi = TIMER_DATA(localTimer + 1);
    int lo2 = TIMER_DATA(localTimer);
    int hi2 = TIMER_DATA(localTimer + 1);

    if (lo2 < lo)
    {
        lo = lo2;
        hi = hi2;
    }

    TIMER_CR(localTimer) = 0;
    TIMER_CR(localTimer + 1) = 0;

    return lo | hi << 16;
}
