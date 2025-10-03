// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/debug.h
///
/// @brief Currently only used to send debug messages to NO$GBA debug window.
///
/// This functionality is best accessed via the console stdio integration.
/// - @ref console.h "Debug Messages via stdio"
///
/// On the ARM7 stderr is setup to print to the no$gba debug console:
/// ```
/// fprintf(stderr,"ARM7 %%scanline%%\n");
/// ```
/// On the ARM9 stderr is directed to the console by default, but it's possible
/// to direct it to the no$gba debug console:
/// ```
/// consoleDebugInit(DebugDevice_NOCASH);
/// fprintf(stderr, "ARM9 %%scanline%%\n");
/// ```
///
/// Messages can be up to 120 characters long. They can also use special
/// parameters:
///
///     r0,r1,r2,...,r15  show register content (displayed as 32bit Hex number)
///     sp,lr,pc          alias for r13,r14,r15
///     scanline          show current scanline number
///     frame             show total number of frames since coldboot
///     totalclks         show total number of clock cycles since coldboot
///     lastclks          show number of cycles since previous lastclks (or zeroclks)
///     zeroclks          resets the 'lastclks' counter

#ifndef LIBNDS_NDS_DEBUG_H__
#define LIBNDS_NDS_DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

/// Emulation ID (16 bytes, eg. "no$gba v2.7", padded with 0x20)
#define REG_NOCASH_EMULATOR_ID  ((vu8*)0x4FFFA00)
/// String out (raw)
#define REG_NOCASH_STR_RAW      (*(vu32*)0x4FFFA10)
/// String Out (with %param's)
#define REG_NOCASH_STR_PARAM    (*(vu32*)0x4FFFA14)
/// String Out (with %param's, plus linefeed)
#define REG_NOCASH_STR_PARAM_LF (*(vu32*)0x4FFFA18)
/// Char Out (nocash)
#define REG_NOCASH_CHAR         (*(vu32*)0x4FFFA1C)
/// Clock Cycles (64 bit)
#define REG_NOCASH_CLOCK_CYCLES (*(vu64*)0x4FFFA20)

/// Send a message to the no$gba debug window.
///
/// @param message
///     The message to send.
/// @param len
///     Length of the message.
void nocashWrite(const char *message, int len);

/// Send a NULL-terminated message to the no$gba debug window.
///
/// @param message
///     The message to send (120 characters max).
void nocashMessage(const char *message);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_DEBUG_H__
