// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBC_PRIVATE_H__
#define LIBC_PRIVATE_H__

#include <time.h>

void __libnds_exit(int rc);

extern time_t *punixTime;

#endif // LIBC_PRIVATE_H__
