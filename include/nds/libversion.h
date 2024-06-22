// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_LIBVERSION_H__
#define LIBNDS_NDS_LIBVERSION_H__

#define _BLOCKSDS_MAJOR_ 1
#define _BLOCKSDS_MINOR_ 3
#define _BLOCKSDS_PATCH_ 0

#define _BLOCKSDS_STRICT_CURRENT_ (((_BLOCKSDS_MAJOR_) * 10000) + ((_BLOCKSDS_MINOR_) * 100) + (_BLOCKSDS_PATCH_))

// The below defines mark the version of libnds with which BlocksDS tries to be compatible.

#define _LIBNDS_MAJOR_ 1
#define _LIBNDS_MINOR_ 8
#define _LIBNDS_PATCH_ 2

#define _LIBNDS_STRING "BlocksDS " _BLOCKSDS_MAJOR_ "." _BLOCKSDS_MINOR_ "." _BLOCKSDS_PATCH_ " (like libnds " _LIBNDS_MAJOR_ "." _LIBNDS_MINOR_ "." _LIBNDS_PATCH_ ")"

#endif // LIBNDS_NDS_LIBVERSION_H__
