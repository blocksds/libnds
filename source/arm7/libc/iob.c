// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#include <stdio.h>

#include "common/libnds_internal.h"

static FILE __stderr = FDEV_SETUP_STREAM(nocash_putc_buffered, NULL, NULL,
                                         _FDEV_SETUP_WRITE);

FILE *const stderr = &__stderr;
