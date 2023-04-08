// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Jason Rogers (dovoto)

#include <nds/arm9/decompress.h>
#include <nds/bios.h>
#include <nds/arm9/sassert.h>

static int getHeader(uint8 *source, uint16 *dest, uint32 arg) {
	(void)dest;
	(void)arg;

	return *(uint32*)source;
}

static uint8 readByte(uint8 *source) {
	return *source;
}

TDecompressionStream decomStream = {
	getHeader,
	0,
	readByte
};

void decompress(const void* data, void* dst, DecompressType type)
{
	switch(type)
	{
		case LZ77Vram:
			swiDecompressLZSSVram((void*)data, (void*)dst, 0, &decomStream);
			break;
		case LZ77:
			swiDecompressLZSSWram((void*)data, (void*)dst);
			break;
		case HUFF:
			swiDecompressHuffman((void*)data, (void*)dst, 0, &decomStream);
			break;
		case RLE:
			swiDecompressRLEWram((void*)data, (void*)dst);
			break;
		case RLEVram:
			swiDecompressRLEVram((void*)data, (void*)dst, 0, &decomStream);
			break;
		default:
			break;
	}
}

void decompressStream(const void* data, void* dst, DecompressType type, getByteCallback readCB, getHeaderCallback getHeaderCB)
{
#ifdef ARM9
	sassert(type != LZ77 && type != RLE, "LZ77 and RLE do not support streaming, use Vram versions");
#endif


	TDecompressionStream decompresStream =
	{
		getHeaderCB,
		0,
		readCB
	};

	switch(type)
	{
		case LZ77Vram:
			swiDecompressLZSSVram((void*)data, (void*)dst, 0, &decompresStream);
			break;
		case HUFF:
			swiDecompressHuffman((void*)data, (void*)dst, 0, &decompresStream);
			break;
		case RLEVram:
			swiDecompressRLEVram((void*)data, (void*)dst, 0, &decompresStream);
			break;
		default:
			break;
	}
}
