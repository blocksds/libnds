// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_ARM9_EXCEPTIONS_H__
#define LIBNDS_NDS_ARM9_EXCEPTIONS_H__

#include <nds/cpu.h>
#include <nds/ndstypes.h>

/// @file
///
/// @brief Functions to handle hardware exceptions.

#define EXCEPTION_VECTOR (*(VoidFn *)(0x2FFFD9C))

#ifdef __cplusplus
extern "C" {
#endif

extern VoidFn exceptionC;
extern u32 exceptionStack;

/// Array with a copy of all the registers of when the exception occured.
extern s32 exceptionRegisters[];

/// Default exception handler of libnds
void enterException(void);

/// Sets a custom hardware exception handler.
///
/// @param handler Exception handler routine.
void setExceptionHandler(VoidFn handler);

/// Sets the default hardware exception handler.
void defaultExceptionHandler(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_EXCEPTIONS_H__
