// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Jason Rogers (dovoto)

/*! \file decompress.h
    \brief wraps the bios decompress functionality into something a bit easier to deal with.
*/


#ifndef NDS_DECOMPRESS
#define NDS_DECOMPRESS

#include <nds/ndstypes.h>
#include <nds/bios.h>


//! the types of decompression available.
typedef enum
{
   LZ77, 		//!< LZ77 decompression.
   LZ77Vram,	//!< vram safe LZ77 decompression.
   HUFF,		//!< vram safe huff decompression.
   RLE,			//!< run length encoded decompression.
   RLEVram 		//!< vram safe run length encoded decompression.
}DecompressType;

#ifdef __cplusplus
extern "C" {
#endif

/*!
	\brief decompresses data using the suported type
	\param dst the destination to decompress to
	\param data the data to decompress
	\param type the type of data to decompress
*/
void decompress(const void* data, void* dst, DecompressType type);

/*!
	\brief decompresses data using the suported type (only LZ77Vram, HUFF, and RLEVram support streaming)
	\param dst the destination to decompress to.
	\param data the data to decompress.
	\param type the type of data to decompress.
	\param readCB a callback to read the next byte of data.
	\param getHeaderCB a callback to read the 32 byte header.
*/
void decompressStream(const void* data, void* dst, DecompressType type, getByteCallback readCB, getHeaderCallback getHeaderCB);

#ifdef __cplusplus
}
#endif

#endif

