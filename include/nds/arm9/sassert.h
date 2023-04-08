// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2013 Michael Theall (mtheall)
// Copyright (C) 2013 Jason Rogers (dovoto)
// Copyright (C) 2013 Dave Murphy (WinterMute)

// Definitons for DS assertions

/*! \file sassert.h
	\brief Simple assertion with a message conplies to nop if NDEBUG is defined
*/

#ifndef _sassert_h_
#define _sassert_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "_ansi.h"

#undef sassert

#ifdef NDEBUG            /* required by ANSI standard */
#define sassert(e, ...) ((void)0)
#else
//! Causes a blue screen of death if e is not true with the msg "msg" displayed
#define sassert(e,...) ((e) ? (void)0 : __sassert (__FILE__, __LINE__, #e, __VA_ARGS__))

#endif /* NDEBUG */

void __sassert(const char *fileName, int lineNumber, const char* conditionString, const char* format, ...)
__attribute__((format(printf,4,5)));

#ifdef __cplusplus
}
#endif

#endif // _sassert_h_
