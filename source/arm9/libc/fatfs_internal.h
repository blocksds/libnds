// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef FATFS_INTERNAL_H__
#define FATFS_INTERNAL_H__

#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <time.h>

#include "ff.h"

int fatfs_error_to_posix(FRESULT error);
time_t fatfs_fattime_to_timestamp(uint16_t ftime, uint16_t fdate);
uint32_t fatfs_timestamp_to_fattime(struct tm *stm);

#endif // FATFS_INTERNAL_H__
