// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2020 Gericom
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBTEAK_TEAK_H__
#define LIBTEAK_TEAK_H__

/// @file teak/teak.h
///
/// @brief General utilities

#include <teak/ahbm.h>
#include <teak/apbp.h>
#include <teak/btdmp.h>
#include <teak/cpu.h>
#include <teak/dma.h>
#include <teak/icu.h>
#include <teak/timer.h>
#include <teak/types.h>

/// Initialize Teak peripherals to a sensible state.
void teakInit(void);

#endif // LIBTEAK_TEAK_H__
