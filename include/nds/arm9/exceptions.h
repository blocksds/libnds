// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_ARM9_EXCEPTIONS_H__
#define LIBNDS_NDS_ARM9_EXCEPTIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/arm9/exceptions.h
///
/// @brief Functions to handle hardware exceptions.
///
/// Check https://www.problemkaputt.de/gbatek.htm#biosramusage for more
/// information.

#include <nds/cpu.h>
#include <nds/ndstypes.h>

/// NDS9 BIOS debug exception vector, or 0 for no handler (mirror)
#define EXCEPTION_VECTOR    (*(VoidFn *)(0x2FFFD9C))

/// NDS9 BIOS debug exception stack top (mirror)
#define EXCEPTION_STACK_TOP ((uint32_t *)0x2FFFD9C)

/// Pointer to the user exception handler, called from the exception handler of
/// libnds.
extern VoidFn exceptionC;

/// Stack reserved for the user exception handler, allocated by the exception
/// handler of libnds.
extern u32 exceptionStack;

/// Array with a copy of all the registers of when the exception occured.
extern s32 exceptionRegisters[];

/// Default exception handler of libnds
void enterException(void);

/// Sets a custom hardware exception handler.
///
/// @param handler
///     Exception handler routine.
void setExceptionHandler(VoidFn handler);

/// Sets the default debug hardware exception handler.
///
/// This handler prints a lot of information, like the state of the CPU
/// registers when the CPU crashed.
void defaultExceptionHandler(void);

/// Sets the release hardware exception handler.
///
/// This is similar to defaultExceptionHandler(), but it only prints a minimal
/// error message, and it uses a lot less code to do it.
void releaseExceptionHandler(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_EXCEPTIONS_H__
