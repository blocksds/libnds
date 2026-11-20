// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef LIBNDS_THREADS_H__
#define LIBNDS_THREADS_H__

#include <nds/cothread.h>

void cothread_list_active_add_after_first_ctx(volatile cothread_info_t *ctx);

cothread_t cothread_create_internal(cothread_info_t *ctx,
                                    int (*entrypoint)(void *), void *arg,
                                    void *stack_base, void *stack_top,
                                    void *tls, uint16_t flags);

#endif // LIBNDS_THREADS_H__
