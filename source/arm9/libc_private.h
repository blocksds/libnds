// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef ARM9_LIBC_PRIVATE_H__
#define ARM9_LIBC_PRIVATE_H__

#include "../common/libc_private.h"

typedef ssize_t (* fn_write_ptr)(const char *, size_t);
extern fn_write_ptr libnds_stdout_write, libnds_stderr_write;

#endif // ARM9_LIBC_PRIVATE_H__
