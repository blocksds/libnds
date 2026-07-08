// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#include <nds/cpu_asm.h>
#include <nds/interrupts.h>
#include <nds/timers.h>

// The bottom 16 bits are left as 0 so that we can create the full timer value
// by OR'ing the tick count and a timer value read from the timer register.
static volatile uint64_t libnds_tick_count;

ARM_CODE ITCM_CODE uint64_t systemCounterGetTicks(void)
{
    uint32_t old_cpsr;

    // Start critical section. Disable interrupts
    asm volatile (
        "mrs %0, cpsr\n"
        "orr r1, %0, %1\n"
        "msr cpsr, r1\n"
        : "=r" (old_cpsr) : "n" (CPSR_FLAG_IRQ_DIS) : "r1", "memory");

    uint64_t count = libnds_tick_count;

    uint16_t timer1 = TIMER_DATA(LIBNDS_TIMER_SYSTEM_COUNTER);

    COMPILER_MEMORY_BARRIER();

    uint16_t reg_if = REG_IF;

    COMPILER_MEMORY_BARRIER();

    uint16_t timer2 = TIMER_DATA(LIBNDS_TIMER_SYSTEM_COUNTER);

    // Leave critical section. Restore previous CPSR value
    asm volatile (
        "msr cpsr, %0\n"
        :: "r" (old_cpsr) : "memory");

    if ((reg_if & IRQ_TIMER(LIBNDS_TIMER_SYSTEM_COUNTER)) || (timer2 < timer1))
        count += 1 << 16;

    return count | timer2;
}

static void system_counter_handler(void)
{
    libnds_tick_count += 1 << 16;
}

void systemCounterSetup(void)
{
    timerStart(LIBNDS_TIMER_SYSTEM_COUNTER,
               ClockDivider_64, 0,
               system_counter_handler);
}
