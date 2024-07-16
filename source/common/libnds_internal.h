// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005-2008 Dave Murphy (WinterMute)
// Copyright (c) 2023 Antonio Niño Díaz

// Internal variables for libnds

#ifndef COMMON_LIBNDS_INTERNAL_H__
#define COMMON_LIBNDS_INTERNAL_H__

#include <stdio.h>

#include <nds/arm9/input.h>
#include <nds/ndstypes.h>

void setTransferInputData(touchPosition *touch, u16 buttons);
void __libnds_exit(int rc);

extern time_t *punixTime;

int nocash_putc_buffered(char c, FILE *file);
ssize_t nocash_write(const char *ptr, size_t len);

#endif // COMMON_LIBNDS_INTERNAL_H__
