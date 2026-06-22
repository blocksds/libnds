// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Antonio Niño Díaz

#ifndef FCNTL_H__
#define FCNTL_H__

#ifdef __cplusplus
extern "C" {
#endif

// Chosen to match lwIP

// Commands for fnctl
#define F_DUPFD 0 // Duplicate fildes
#define F_GETFD 1 // Get fildes flags (close on exec)
#define F_SETFD 2 // Set fildes flags (close on exec)
#define F_GETFL 3 // Get file flags
#define F_SETFL 4 // Set file flags

// Flag values for open(2) and fcntl(2)
#define O_NONBLOCK  (1 << 0)    // nonblocking I/O
#define O_NDELAY    O_NONBLOCK
#define O_RDONLY    (1 << 1)
#define O_WRONLY    (1 << 2)
#define O_RDWR      (O_RDONLY | O_WRONLY)
#define O_ACCMODE   O_RDWR
#define O_APPEND    (1 << 3)
#define O_CREAT     (1 << 4)
#define O_TRUNC     (1 << 5)
#define O_EXCL      (1 << 6)
#define O_SYNC      (1 << 7)
#define O_ASYNC     (1 << 8)
#define O_NOCTTY    (1 << 9)

int open(const char *, int, ...);
int fcntl(int fd, int op, ...);

#ifdef __cplusplus
}
#endif

#endif // FCNTL_H__
