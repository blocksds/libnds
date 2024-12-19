// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Antonio Niño Díaz

#ifndef EXCEPTIONS_INTERNAL_H__
#define EXCEPTIONS_INTERNAL_H__

#include <nds/ndstypes.h>

extern const char *exceptionMsg;

uint32_t ARMShift(uint32_t value, uint8_t shift);
u32 getExceptionAddress(u32 opcodeAddress, u32 thumbState);

#endif // EXCEPTIONS_INTERNAL_H__
