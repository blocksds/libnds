// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef _exceptions_h_
#define _exceptions_h_

#include <nds/cpu.h>
#include <nds/ndstypes.h>

/** \file
	\brief functions to handle hardware exceptions.
*/

#define EXCEPTION_VECTOR	(*(VoidFn *)(0x2FFFD9C))

#ifdef __cplusplus
extern "C" {
#endif


extern VoidFn exceptionC[];//shouldn't this be a pointer instead of an array?
extern u32 exceptionStack;

//! an array with a copy of all the registers of when the exception occured.
extern s32 exceptionRegisters[];


void enterException(void);

//! sets a custom hardware exception handler.
void setExceptionHandler(VoidFn handler);

//! sets the default hardware exception handler.
void defaultExceptionHandler(void);

#ifdef __cplusplus
}
#endif

#endif // _exceptions_h_
