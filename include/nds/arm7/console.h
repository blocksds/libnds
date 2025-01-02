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
#include <sys/cdefs.h>

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

/// Adds an unsigned integer to the ring buffer to be printed.
///
/// @param num
///     Unsigned integer to be printed.
/// @param base
///     Base of the number to be used (normally 10 or 16, max is 16).
void consolePrintNumUnsigned(uint32_t num, uint32_t base);

/// Adds a string to the ring buffer to be printed.
///
/// @param str
///     String to be printed.
void consolePuts(const char *str);

/// It adds a formatted string to the buffer.
///
/// This version is a super minimalistic printf(). Supported flags:
/// - %c: Character.
/// - $d: Signed decimal 32-bit integer.
/// - %s: String.
/// - $u: Unsigned decimal 32-bit integer.
/// - %x: Hexadecimal 32-bit integer.
///
/// @param fmt
///     Formatted string.
///
/// @return
///     It returns 0 on success, -1 if there are unsuported flags.
int consolePrintf(const char *fmt, ...) __printflike(1, 2);

/// Sends a message to the ARM9 to print the contents stored in the buffer.
void consoleFlush(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_CONSOLE_H__

