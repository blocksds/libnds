// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)

/// @file nds/arm9/decompress.h
///
/// @brief Wraps the bios decompress functionality into something a bit easier
/// to use.

#ifndef LIBNDS_NDS_ARM9_DECOMPRESS_H__
#define LIBNDS_NDS_ARM9_DECOMPRESS_H__

#include <nds/bios.h>
#include <nds/ndstypes.h>

/// The types of decompression available.
typedef enum
{
   LZ77,        ///< LZ77 decompression.
   LZ77Vram,    ///< VRAM safe LZ77 decompression.
   HUFF,        ///< VRAM safe huff decompression.
   RLE,         ///< Run length encoded decompression.
   RLEVram      ///< VRAM safe run length encoded decompression.
}DecompressType;

#ifdef __cplusplus
extern "C" {
#endif

/// Decompresses data using the suported type.
///
/// @param dst Destination to decompress to.
/// @param data Data to decompress.
/// @param type Type of data to decompress.
void decompress(const void *data, void *dst, DecompressType type);

/// Decompresses data using the suported type (only LZ77Vram, HUFF, and RLEVram
/// support streaming)
///
/// @param dst Destination to decompress to.
/// @param data Data to decompress.
/// @param type Type of data to decompress.
/// @param readCB A callback to read the next byte of data.
/// @param getHeaderCB A callback to read the 32 byte header.
void decompressStream(const void *data, void *dst, DecompressType type,
                      getByteCallback readCB, getHeaderCallback getHeaderCB);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_DECOMPRESS_H__
