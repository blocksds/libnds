// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef ARM9_LIBNDS_INTERNAL_H__
#define ARM9_LIBNDS_INTERNAL_H__

#include "common/libnds_internal.h"

typedef ssize_t (* fn_write_ptr)(const char *, size_t);
extern fn_write_ptr libnds_stdout_write, libnds_stderr_write;

#endif // ARM9_LIBNDS_INTERNAL_H__
