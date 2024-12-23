// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Antonio Niño Díaz

#ifndef LIBNDS_NDS_ARM7_CONSOLE_H__
#define LIBNDS_NDS_ARM7_CONSOLE_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/arm7/console.h
///
/// @brief API to send console messages to the ARM9 from the ARM7.
///
/// You need to setup this console by calling consoleArm7Setup() on the ARM9.

#include <stdbool.h>

/// Checks if the console has been setup by the ARM9 or not.
///
/// @return
///     Returns true if the console is setup, false if not.
bool consoleIsSetup(void);

/// Adds a character to the ring buffer to be printed.
///
/// If the buffer is full, this function will send a flush command to the ARM9
/// and it will wait until there is space to add a new character.
///
/// @param c
///     Character to be printed.
///
/// @return
///     It returns 0 on success, or an error code if the console hasn't been
///     initialized.
int consolePrintChar(char c);

/// Sends a message to the ARM9 to print the contents stored in the buffer.
void consoleFlush(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_CONSOLE_H__

