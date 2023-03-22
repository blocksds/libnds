// SPDX-License-Identifier: Zlib
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <stdbool.h>

#include <nds/cothread.h>

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
        cothread_yield();
}

void comutex_release(comutex_t *mutex)
{
    *mutex = 0;
}
