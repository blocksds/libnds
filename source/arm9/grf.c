// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Antonio Niño Díaz

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nds/arm9/decompress.h>
#include <nds/arm9/grf.h>

// General file structure:
//
// "RIFF" # {
//     "GRF " # {
//         "HDR " # { header info }
//         "GFX " # { gfx data }
//         "MAP " # { map data }
//         "MTIL" # { metatile data }
//         "MMAP" # { metamap data }
//         "PAL " # { palette data }
//     }
// }
//
// The only mandatory chunk inside the "GRF " is "HDR ".

typedef struct {
    uint32_t    id;
    uint32_t    size;
    uint8_t     data[];
} riff_chunk_t;

#define CHUNK_ID(a, b, c, d) \
    ((uint32_t)((a) | ((b) << 8) | ((c) << 16) | ((d) << 24)))

#define ID_RIFF     CHUNK_ID('R', 'I', 'F', 'F')
#define ID_GRF      CHUNK_ID('G', 'R', 'F', ' ')
#define ID_HDR      CHUNK_ID('H', 'D', 'R', ' ')
#define ID_GFX      CHUNK_ID('G', 'F', 'X', ' ')
#define ID_MAP      CHUNK_ID('M', 'A', 'P', ' ')
#define ID_MTIL     CHUNK_ID('M', 'T', 'I', 'L')
#define ID_MMAP     CHUNK_ID('M', 'M', 'A', 'P')
#define ID_PAL      CHUNK_ID('P', 'A', 'L', ' ')

// Extracts a GRF item
static int grfExtract(const void *src, void **dst)
{
    if ((src == NULL) || (dst == NULL))
        return -1;

    // The header of this data is the header used for all GBA/NDS BIOS
    // decompression routines. Uncompressed chunks also use the same format for
    // consistency.
    uint32_t header = *(uint32_t *)src;
    uint32_t size = header >> 8;

    // If the user has already provided a pointer, use it. If not, allocate mem
    if (*dst == NULL)
    {
        *dst = malloc(size);
        if (*dst == NULL)
            return -2;
    }

    switch (header & 0xF0)
    {
        case 0x00: // No compression
            swiCopy((uint8_t *)src + 4, *dst, COPY_MODE_HWORD | COPY_MODE_COPY | (size >> 1));
            return 0;
        case 0x10: // LZ77
            decompress(src, *dst, LZ77Vram);
            return 0;
        case 0x20: // Huffman
            decompress(src, *dst, HUFF);
            return 0;
        case 0x30: // RLE
            decompress(src, *dst, RLEVram);
            return 0;
        default:
            return -2;
    }
}

int grfLoad(const void *src, grf_header_t *header, void **gfxDst,
            void **mapDst, void **palDst, void **mtilDst, void **mmapDst)
{
    if (src == NULL)
        return -1;

    const riff_chunk_t *riff_chunk = src;

    if (riff_chunk->id != ID_RIFF)
        return -2;

    uint32_t riff_size = riff_chunk->size;

    const riff_chunk_t *grf_chunk = (const riff_chunk_t *)&(riff_chunk->data[0]);

    if (grf_chunk->id != ID_GRF)
        return -3;

    uint32_t grf_size = grf_chunk->size;

    // Ensure that both sizes are consistent
    if (riff_size != grf_size + 8)
        return -4;

    uintptr_t ptr = (uintptr_t)&(grf_chunk->data[0]);
    uintptr_t end = (uintptr_t)src + riff_size + 8;

    while (ptr < end)
    {
        const riff_chunk_t *chunk = (const riff_chunk_t *)ptr;

        uint32_t id = chunk->id;
        uint32_t size = chunk->size;
        const void *data = (const void *)&(chunk->data[0]);

        ptr += size + 8;

        int ret = -5;

        switch (id)
        {
            case ID_HDR:
                // Read header chunk
                if (size != sizeof(grf_header_t))
                    return -6;
                memcpy(header, data, size);
                ret = 0;
                break;
            case ID_GFX:
                if (gfxDst)
                    ret = grfExtract(data, gfxDst);
                break;
            case ID_MAP:
                if (mapDst)
                    grfExtract(data, mapDst);
                break;
            case ID_MTIL:
                if (mtilDst)
                    grfExtract(data, mtilDst);
                break;
            case ID_MMAP:
                if (mmapDst)
                    grfExtract(data, mmapDst);
                break;
            case ID_PAL:
                if (palDst)
                    grfExtract(data, palDst);
                break;
            default:
                break;
        }

        if (ret != 0)
            return ret;
    }

    return 0;
}
