// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2024 Antonio Niño Díaz

#ifndef ARM9_LIBNDS_INTERNAL_H__
#define ARM9_LIBNDS_INTERNAL_H__

#include <time.h>

#include <nds/arm9/console.h>
#include <nds/arm9/input.h>

extern ConsoleOutFn libnds_stdout_write, libnds_stderr_write;

void setTransferInputData(touchPosition *touch, u16 buttons);

extern time_t *punixTime;

#endif // ARM9_LIBNDS_INTERNAL_H__
