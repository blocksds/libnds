// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

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

/// Set bit CPSR.I to disable interrupts.
///
/// @return
///     A value that must be restored by cpsrCriticalSectionEnd().
uint32_t cpsrCriticalSectionStart(void);

/// Restore value of CPSR.I.
///
/// @param old_cpsr
///     A value obtained with cpsrCriticalSectionStart().
void cpsrCriticalSectionEnd(uint32_t old_cpsr);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_CPU_H__
