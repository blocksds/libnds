// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include <nds/cothread.h>

bool cosema_init(cosema_t *sema, uint32_t init_val)
{
    *sema = init_val;
    return true;
}

void cosema_signal(cosema_t *sema)
{
    *sema = *sema + 1;
    cothread_send_signal(cosema_to_signal_id(sema));
}

bool cosema_try_wait(cosema_t *sema)
{
    if (*sema > 0)
    {
        *sema = *sema - 1;
        return true;
    }

    return false;
}

void cosema_wait(cosema_t *sema)
{
    while (cosema_try_wait(sema) == false)
        cothread_yield_signal(cosema_to_signal_id(sema));
}

