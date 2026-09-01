// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2013 Michael Theall (mtheall)
// Copyright (C) 2013 Jason Rogers (dovoto)
// Copyright (C) 2013 Dave Murphy (WinterMute)

/// @file nds/arm9/sassert.h
///
/// @brief Simple assertion with a message that disappears if NDEBUG is defined.

#ifndef LIBNDS_NDS_ARM9_SASSERT_H__
#define LIBNDS_NDS_ARM9_SASSERT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/cdefs.h>

#include <nds/ndstypes.h>

#undef sassert

#ifdef NDEBUG // Required by ANSI standard
#define sassert(e, ...) ((void)0)
#else
/// Causes a blue screen of death if e is not true with the msg "msg" displayed.
// This macro can't be used from IRQ handlers.
#define sassert(e, ...) ((e) ? (void)0 : __sassert (__FILE__, __LINE__, #e, __VA_ARGS__))
#endif // NDEBUG

// This function can be called from IRQ handlers
LIBNDS_NORETURN
void __sassert_nofmt(const char *fileName, int lineNumber, const char *conditionString,
                     const char *format);

// This function can't be called from IRQ handlers
LIBNDS_NORETURN LIBNDS_PRINTFLIKE(4, 5)
void __sassert(const char *fileName, int lineNumber, const char *conditionString,
               const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_SASSERT_H__
