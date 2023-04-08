// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright (C) 2016 Dave Murphy (WinterMute)

#ifndef NDS_NAND_INCLUDE
#define NDS_NAND_INCLUDE

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

bool nand_ReadSectors(sec_t sector, sec_t numSectors,void* buffer);
bool nand_WriteSectors(sec_t sector, sec_t numSectors,const void* buffer);
ssize_t nand_GetSize(void);

#ifdef __cplusplus
}
#endif
#endif // NDS_NAND_INCLUDE
