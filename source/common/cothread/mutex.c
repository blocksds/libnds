// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include <nds/cothread.h>
#include <nds/interrupts.h>

bool comutex_init(comutex_t *mutex)
{
    *mutex = 0;

    COMPILER_MEMORY_BARRIER();

    return true;
}

bool comutex_try_acquire(comutex_t *mutex)
{
    bool acquired;

    int oldIME = enterCriticalSection();

    COMPILER_MEMORY_BARRIER();

    if (*mutex != 0)
    {
        acquired = false;
    }
    else
    {
        *mutex = 1;
        acquired = true;
    }

    COMPILER_MEMORY_BARRIER();

    leaveCriticalSection(oldIME);

    return acquired;
}

void comutex_acquire(comutex_t *mutex)
{
    while (comutex_try_acquire(mutex) == false)
        cothread_yield_signal(comutex_to_signal_id(mutex));
}

void comutex_release(comutex_t *mutex)
{
    COMPILER_MEMORY_BARRIER();

    *mutex = 0;

    COMPILER_MEMORY_BARRIER();

    cothread_send_signal(comutex_to_signal_id(mutex));
}
