// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_CPU_H__
#define LIBNDS_NDS_CPU_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <nds/cpu_asm.h>

/// Sets the CPU status register.
uint32_t setCPSR(void);

/// Returns the cpu status register.
uint32_t getCPSR(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_CPU_H__
