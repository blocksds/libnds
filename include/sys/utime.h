// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef SYS_UTIME_H__
#define SYS_UTIME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct utimbuf
{
    time_t actime;
    time_t modtime;
};

int utime(const char *filename, const struct utimbuf *times);

#ifdef __cplusplus
}
#endif

#endif // SYS_UTIME_H__
