// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_CPU_H__
#define LIBNDS_NDS_CPU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <nds/cpu_asm.h>

/// Sets the CPU status register.
void setCPSR(uint32_t);

/// Returns the cpu status register.
uint32_t getCPSR(void);

/// Enable interrupts in CPSR
void cpsrEnableInterrupts(void);

/// Disable interrupts in CPSR
void cpsrDisableInterrupts(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_CPU_H__
