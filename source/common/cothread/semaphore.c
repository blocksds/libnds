// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include <nds/cothread.h>
#include <nds/interrupts.h>

bool cosema_init(cosema_t *sema, uint32_t init_val)
{
    COMPILER_MEMORY_BARRIER();

    *sema = init_val;

    COMPILER_MEMORY_BARRIER();

    return true;
}

void cosema_signal(cosema_t *sema)
{
    int oldIME = enterCriticalSection();

    COMPILER_MEMORY_BARRIER();

    *sema = *sema + 1;

    COMPILER_MEMORY_BARRIER();

    leaveCriticalSection(oldIME);

    cothread_send_signal(cosema_to_signal_id(sema));
}

bool cosema_try_wait(cosema_t *sema)
{
    bool success;

    int oldIME = enterCriticalSection();

    COMPILER_MEMORY_BARRIER();

    if (*sema > 0)
    {
        *sema = *sema - 1;
        success = true;
    }
    else
    {
        success = false;
    }

    COMPILER_MEMORY_BARRIER();

    leaveCriticalSection(oldIME);

    return success;
}

void cosema_wait(cosema_t *sema)
{
    while (cosema_try_wait(sema) == false)
        cothread_yield_signal(cosema_to_signal_id(sema));
}

