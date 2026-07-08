// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#include <unistd.h>

#include <nds/cothread.h>
#include <nds/system_counter.h>

// This implementation needs to be improved when system counter tasks are
// implemented.

int usleep(useconds_t usec)
{
    uint64_t now = systemCounterGetTicks();
    uint64_t end = now + systemCounterUsecsToTicks(usec);

    while (1)
    {
        now = systemCounterGetTicks();
        if (now >= end)
            return 0;

        // Only yield if we have to wait for a long time
        if ((end - now) > 100)
            cothread_yield();
    }
}

unsigned int sleep(unsigned int seconds)
{
    return usleep(seconds * 1000000);
}
