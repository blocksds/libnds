// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include <nds/cothread.h>

bool comutex_init(comutex_t *mutex)
{
    *mutex = 0;
    return true;
}

bool comutex_try_acquire(comutex_t *mutex)
{
    if (*mutex != 0)
        return false;

    *mutex = 1;
    return true;
}

void comutex_acquire(comutex_t *mutex)
{
    while (comutex_try_acquire(mutex) == false)
        cothread_yield_signal(comutex_to_signal_id(mutex));
}

void comutex_release(comutex_t *mutex)
{
    *mutex = 0;
    cothread_send_signal(comutex_to_signal_id(mutex));
}
