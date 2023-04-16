// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef ARM7_LIBNDS_INTERNAL_H__
#define ARM7_LIBNDS_INTERNAL_H__

#include <nds/ndstypes.h>

void storageMsgHandler(int bytes, void *user_data);
void storageValueHandler(u32 value, void *user_data);
void firmwareMsgHandler(int bytes, void *user_data);

#endif // ARM7_LIBNDS_INTERNAL_H__
