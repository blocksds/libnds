// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)

#include <assert.h>
#include <stdlib.h>

#include <nds/bios.h>
#include <nds/decompress.h>

static int decompress_get_header(uint8_t *source, uint16_t *dest, uint32_t arg)
{
    (void)dest;
    (void)arg;

    return *(uint32_t *)source;
}

static uint8_t decompress_read_8(uint8_t *source)
{
    return *source;
}

static uint16_t decompress_read_16(uint16_t *source)
{
    return *source;
}

static uint32_t decompress_read_32(uint32_t *source)
{
    return *source;
}

TDecompressionStream decomStream =
{
    decompress_get_header,
    NULL, // The close callback can be omitted
    decompress_read_8,
    decompress_read_16,
    decompress_read_32
};

void decompress(const void *data, void *dst, DecompressType type)
{
    switch (type)
    {
        case LZ77Vram:
            swiDecompressLZSSVram(data, dst, 0, &decomStream);
            break;
        case LZ77:
            swiDecompressLZSSWram(data, dst);
            break;
        case HUFF:
        {
            // This temporary buffer is allocated in the stack, in DTCM, but
            // that's okay because the ARM9 BIOS can access DTCM.
            uint32_t temp[512 / sizeof(uint32_t)];
            swiDecompressHuffman(data, dst, (uintptr_t)&temp[0], &decomStream);
            break;
        }
        case RLE:
            swiDecompressRLEWram(data, dst);
            break;
        case RLEVram:
            swiDecompressRLEVram(data, dst, 0, &decomStream);
            break;
        default:
            break;
    }
}

void decompressStream(const void *data, void *dst, DecompressType type,
                      getByteCallback readCB, getHeaderCallback getHeaderCB)
{
    // LZ77 and RLE do not support streaming, use VRAM versions
    assert(type != LZ77 && type != RLE);

    // HUFF not supported, use decompresStreamStruct()
    assert(type != HUFF);

    TDecompressionStream decompresStream =
    {
        getHeaderCB,
        NULL,
        readCB,
        NULL,
        NULL // Only required for Huffman
    };

    switch (type)
    {
        case LZ77Vram:
            swiDecompressLZSSVram(data, dst, 0, &decompresStream);
            break;
        case RLEVram:
            swiDecompressRLEVram(data, dst, 0, &decompresStream);
            break;
        default:
            break;
    }
}

void decompressStreamStruct(const void *data, void *dst, DecompressType type,
                            void *param, TDecompressionStream *ds)
{
    // LZ77 and RLE do not support streaming, use VRAM versions
    assert(type != LZ77 && type != RLE);

    // getSize() and readByte() callbacks are required
    assert((ds->getSize != NULL) && (ds->readByte != NULL));

    switch (type)
    {
        case LZ77Vram:
            swiDecompressLZSSVram(data, dst, (uintptr_t)param, ds);
            break;
        case HUFF:
        {
            assert(param != NULL); // Temporary buffer required for HUFF
            assert(ds->readWord != NULL); // readWord() callback required for HUFF

            swiDecompressHuffman(data, dst, (uintptr_t)param, ds);
            break;
        }
        case RLEVram:
            swiDecompressRLEVram(data, dst, (uintptr_t)param, ds);
            break;
        default:
            break;
    }
}
