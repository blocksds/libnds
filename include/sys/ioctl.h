// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef SYS_IOCTL_H__
#define SYS_IOCTL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define IOC_OUT         0x40000000UL // Copy out parameters
#define IOC_IN          0x80000000UL // Copy in parameters
#define _IOR(x,y,t)     ((long)(IOC_OUT | (sizeof(t) << 16) | ((x) << 8) | (y)))
#define _IOW(x,y,t)     ((long)(IOC_IN | (sizeof(t) << 16) | ((x) << 8) | (y)))

#define FIONREAD        _IOR('f', 127, unsigned long) // Get number of bytes to read
#define FIONBIO         _IOW('f', 126, unsigned long) // Set/clear non-blocking I/O

int ioctl(int s, unsigned long op, ...);

#ifdef __cplusplus
}
#endif

#endif // SYS_IOCTL_H__
