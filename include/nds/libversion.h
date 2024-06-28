// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_LIBVERSION_H__
#define LIBNDS_NDS_LIBVERSION_H__

#define BLOCKSDS_MAJOR_VERSION 1
#define BLOCKSDS_MINOR_VERSION 3
#define BLOCKSDS_PATCH_VERSION 0

#define BLOCKSDS_VERSIONNUM(major, minor, patch) (((major) * 10000) + ((minor) * 100) + (patch))
#define BLOCKSDS_VERSIONNUM_CURRENT BLOCKSDS_VERSIONNUM(BLOCKSDS_MAJOR_VERSION, BLOCKSDS_MINOR_VERSION, BLOCKSDS_PATCH_VERSION)

// The below defines mark the version of libnds with which BlocksDS tries to be compatible.

#define _LIBNDS_MAJOR_ 1
#define _LIBNDS_MINOR_ 8
#define _LIBNDS_PATCH_ 2

#define _LIBNDS_STRING "BlocksDS " _BLOCKSDS_MAJOR_ "." _BLOCKSDS_MINOR_ "." _BLOCKSDS_PATCH_ " (like libnds " _LIBNDS_MAJOR_ "." _LIBNDS_MINOR_ "." _LIBNDS_PATCH_ ")"

#endif // LIBNDS_NDS_LIBVERSION_H__
