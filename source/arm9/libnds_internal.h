// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2024 Antonio Niño Díaz

#ifndef ARM9_LIBNDS_INTERNAL_H__
#define ARM9_LIBNDS_INTERNAL_H__

#include <time.h>

#include <nds/arm9/console.h>
#include <nds/arm9/input.h>

extern ConsoleOutFn libnds_stdout_write, libnds_stderr_write;

/**
 * @brief Picolibc stdin reader for the keyboard.
 *
 * Defined as a weak symbol; strong symbol with implementation is only pulled
 * in if the keyboard code is used elsewhere in the binary (to initialize it).
 */
int libnds_stdin_getc_keyboard(FILE *file);

void setTransferInputData(touchPosition *touch, u16 buttons);

extern time_t *punixTime;

#endif // ARM9_LIBNDS_INTERNAL_H__
