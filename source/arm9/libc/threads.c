// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Adrian "asie" Siekierka

#include <errno.h>
#include <nds/cothread.h>
#include <threads.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"

int thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    thrd_t thread = cothread_create((cothread_entrypoint_t) func, arg, 0, 0);

    if (thread < 0)
    {
        return errno == ENOMEM ? thrd_nomem : thrd_error;
    }
    else
    {
        if (thr != NULL)
            *thr = thread;

        return thrd_success;
    }
}

#pragma GCC diagnostic pop

int thrd_join(thrd_t thr, int *res)
{
    while (!cothread_has_joined(thr))
        cothread_yield();

    if (res != NULL)
        *res = cothread_get_exit_code(thr);

    return thrd_success;
}
