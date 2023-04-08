// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright © 2019 Keith Packard

#include <string.h>
#include <stdint.h>

/*
 * The TLS block consists of initialized data immediately followed by
 * zero filled data
 *
 * These addresses must be defined by the loader configuration file
 */

extern char __tdata_start[];
extern char __tdata_size[];

extern char __tbss_start[];
extern char __tbss_size[];

extern char __tls_start[];
extern char __tls_end[];

void _init_tls(void *__tls)
{
    char *tls = __tls;

    char *tdata_start = tls;
    char *tbss_start = tls + (uintptr_t)__tdata_size;

    // Copy tdata
    memcpy(tdata_start, __tdata_start, (uintptr_t)__tdata_size);

    // Clear tbss
    memset(tbss_start, 0, (uintptr_t)__tbss_size);
}
