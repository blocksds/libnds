// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef ARM7_LIBNDS_INTERNAL_H__
#define ARM7_LIBNDS_INTERNAL_H__

#include <nds/ndstypes.h>

void storageMsgHandler(int bytes, void *user_data);
void storageValueHandler(u32 value, void *user_data);
void firmwareMsgHandler(int bytes, void *user_data);

#define LIBNDS_TOUCH_INVALID 0xFFFF

/**
 * @brief Maximum difference used for pixel readouts.
 * 
 * 2.5 pixels * 16 = 40.
 * Use a slightly smaller range as calibration is usually confined to a smaller window than the full 4096.
 */
#define LIBNDS_TOUCH_MAX_DIFF_PIXEL 38

/**
 * @brief Maximum difference used for other readouts.
 *
 * No particular restriction.
 */
#define LIBNDS_TOUCH_MAX_DIFF_OTHER 0xFFFFFF

/**
 * @brief Perform touch filtering on the raw measurements provided.
 */
u16 libnds_touchFilter(u16 *measurements, int max_diff);

#endif // ARM7_LIBNDS_INTERNAL_H__
