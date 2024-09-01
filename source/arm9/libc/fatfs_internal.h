// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef FATFS_INTERNAL_H__
#define FATFS_INTERNAL_H__

#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "ff.h"

int fatfs_error_to_posix(FRESULT error);
uint32_t fatfs_timestamp_to_fattime(struct tm *stm);

#endif // FATFS_INTERNAL_H__
