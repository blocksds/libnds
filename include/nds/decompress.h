// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)

/// @file nds/decompress.h
///
/// @brief Wraps the bios decompress functionality into something a bit easier
/// to use.

#ifndef LIBNDS_NDS_DECOMPRESS_H__
#define LIBNDS_NDS_DECOMPRESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/bios.h>
#include <nds/ndstypes.h>

/// The types of decompression available.
typedef enum
{
    LZ77,       ///< LZ77 decompression.
    LZ77Vram,   ///< VRAM safe LZ77 decompression.
    HUFF,       ///< VRAM safe huff decompression.
    RLE,        ///< Run length encoded decompression.
    RLEVram     ///< VRAM safe run length encoded decompression.
} DecompressType;

/// Decompresses data using the suported type.
///
/// When 'type' is HUFF, this function will allocate 512 bytes in the stack as a
/// temporary buffer.
///
/// @param dst
///     Destination to decompress to.
/// @param data
///     Data to decompress.
/// @param type
///     Type of data to decompress.
void decompress(const void *data, void *dst, DecompressType type);

/// Decompresses data using the suported type.
///
/// Only LZ77Vram, HUFF and RLEVram support streaming, but HUFF isn't supported
/// by this function at all, use decompressStreamStruct() instead.
///
/// @param dst
///     Destination to decompress to.
/// @param data
///     Data to decompress.
/// @param type
///     Type of data to decompress.
/// @param readCB
///     A callback to read the next byte of data.
/// @param getHeaderCB
///     A callback to read the 32 byte header.
void decompressStream(const void *data, void *dst, DecompressType type,
                      getByteCallback readCB, getHeaderCallback getHeaderCB);

/// Decompresses data using the suported type.
///
/// Only LZ77Vram, HUFF and RLEVram support streaming.
///
/// For HUFF, make sure to pass a 512 byte buffer in 'param' to be used as a
/// temporary buffer by the decompression code.
///
/// @param dst
///     Destination to decompress to.
/// @param data
///     Data to decompress.
/// @param type
///     Type of data to decompress.
/// @param param
///     A value to be passed to getHeaderCallback(), or a temporary buffer for
///     HUFF.
/// @param ds
///     A struct with callbacks to be used by the decompression routine.
void decompressStreamStruct(const void *data, void *dst, DecompressType type,
                            void *param, TDecompressionStream *ds);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_DECOMPRESS_H__
