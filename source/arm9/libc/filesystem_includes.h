// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef FILESYSTEM_INCLUDES_H__
#define FILESYSTEM_INCLUDES_H__

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <time.h>
#include <utime.h>

// "dirent.h" defines DIR, but "ff.h" defines a different non-standard one.
// Functions in this file need to use their standard prototypes, so it is needed
// to somehow rename the DIR of "ff.h". It's better to keep the original header
// unmodified so that updating it is easier, so this is a hack to rename it just
// in this compilation unit.
#define DIR DIRff
#include "ff.h"
#include "fatfs_internal.h"
#undef DIR
#include "filesystem_internal.h"
#include "nitrofs_device.h"

// Include "dirent.h" after the FatFs inclusion hack.
#include <dirent.h>

#include "diskio.h"

#endif // FILESYSTEM_INCLUDES_H__
