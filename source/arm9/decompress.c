// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)

#include <nds/arm9/decompress.h>
#include <nds/arm9/sassert.h>
#include <nds/bios.h>

static int getHeader(uint8_t *source, uint16_t *dest, uint32_t arg)
{
    (void)dest;
    (void)arg;

    return *(uint32_t *)source;
}

static uint8_t readByte(uint8_t *source)
{
    return *source;
}

TDecompressionStream decomStream = { getHeader, 0, readByte };

void decompress(const void *data, void *dst, DecompressType type)
{
    switch (type)
    {
        case LZ77Vram:
            swiDecompressLZSSVram((void *)data, (void *)dst, 0, &decomStream);
            break;
        case LZ77:
            swiDecompressLZSSWram((void *)data, (void *)dst);
            break;
        case HUFF:
            swiDecompressHuffman((void *)data, (void *)dst, 0, &decomStream);
            break;
        case RLE:
            swiDecompressRLEWram((void *)data, (void *)dst);
            break;
        case RLEVram:
            swiDecompressRLEVram((void *)data, (void *)dst, 0, &decomStream);
            break;
        default:
            break;
    }
}

void decompressStream(const void *data, void *dst, DecompressType type,
                      getByteCallback readCB, getHeaderCallback getHeaderCB)
{
#ifdef ARM9
    sassert(type != LZ77 && type != RLE,
            "LZ77 and RLE do not support streaming, use Vram versions");
#endif

    TDecompressionStream decompresStream = { getHeaderCB, 0, readCB };

    switch (type)
    {
        case LZ77Vram:
            swiDecompressLZSSVram((void *)data, (void *)dst, 0, &decompresStream);
            break;
        case HUFF:
            swiDecompressHuffman((void *)data, (void *)dst, 0, &decompresStream);
            break;
        case RLEVram:
            swiDecompressRLEVram((void *)data, (void *)dst, 0, &decompresStream);
            break;
        default:
            break;
    }
}
