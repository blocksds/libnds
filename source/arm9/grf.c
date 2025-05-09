// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Antonio Niño Díaz

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nds/arm9/grf.h>
#include <nds/decompress.h>

// General file structure:
//
// "RIFF" # {
//     "GRF " # {
//         "HDRX" # { header info }
//         "GFX " # { gfx data }
//         "MAP " # { map data }
//         "MTIL" # { metatile data }
//         "MMAP" # { metamap data }
//         "PAL " # { palette data }
//     }
// }
//
// The only mandatory chunk inside the "GRF " is "HDRX". "HDR " is an old header
// chunk no longer supported.

typedef struct
{
    uint32_t    id;
    uint32_t    size;
    uint8_t     data[];
} RIFFChunk;

#define CHUNK_ID(a, b, c, d) \
    ((uint32_t)((a) | ((b) << 8) | ((c) << 16) | ((d) << 24)))

#define ID_RIFF     CHUNK_ID('R', 'I', 'F', 'F')
#define ID_GRF      CHUNK_ID('G', 'R', 'F', ' ')
#define ID_HDRX     CHUNK_ID('H', 'D', 'R', 'X')
#define ID_GFX      CHUNK_ID('G', 'F', 'X', ' ')
#define ID_MAP      CHUNK_ID('M', 'A', 'P', ' ')
#define ID_MTIL     CHUNK_ID('M', 'T', 'I', 'L')
#define ID_MMAP     CHUNK_ID('M', 'M', 'A', 'P')
#define ID_PAL      CHUNK_ID('P', 'A', 'L', ' ')

// Extracts a GRF item
static GRFError grfExtract(const void *src, void **dst, size_t *sz)
{
    if ((src == NULL) || (dst == NULL))
        return GRF_NULL_POINTER;

    // The header of this data is the header used for all GBA/NDS BIOS
    // decompression routines. Uncompressed chunks also use the same format for
    // consistency.
    uint32_t header = *(const uint32_t *)src;
    uint32_t size = header >> 8;

    if (sz != NULL)
        *sz = size;

    // If the user has already provided a pointer, use it. If not, allocate mem
    if (*dst == NULL)
    {
        *dst = malloc(size);
        if (*dst == NULL)
            return GRF_NOT_ENOUGH_MEMORY;
    }

    switch (header & 0xF0)
    {
        case 0x00: // No compression
            memcpy(*dst, (const uint8_t *)src + 4, size);
            return GRF_NO_ERROR;
        case 0x10: // LZ77
            decompress(src, *dst, LZ77Vram);
            return GRF_NO_ERROR;
        case 0x20: // Huffman
            decompress(src, *dst, HUFF);
            return GRF_NO_ERROR;
        case 0x30: // RLE
            decompress(src, *dst, RLEVram);
            return GRF_NO_ERROR;
        default:
            return GRF_UNKNOWN_COMPRESSION;
    }
}

GRFError grfLoadMemEx(const void *src, GRFHeader *header,
                      void **gfxDst, size_t *gfxSize,
                      void **mapDst, size_t *mapSize,
                      void **palDst, size_t *palSize,
                      void **mtilDst, size_t *mtilSize,
                      void **mmapDst, size_t *mmapSize)
{
    if (src == NULL)
        return GRF_NULL_POINTER;

    const RIFFChunk *riff_chunk = src;

    if (riff_chunk->id != ID_RIFF)
        return GRF_INVALID_ID_RIFF;

    uint32_t riff_size = riff_chunk->size;

    const RIFFChunk *grf_chunk = (const RIFFChunk *)&(riff_chunk->data[0]);

    if (grf_chunk->id != ID_GRF)
        return GRF_INVALID_ID_GRF;

    uint32_t grf_size = grf_chunk->size;

    // Ensure that both sizes are consistent
    if (riff_size != grf_size + 8)
        return GRF_INCONSISTENT_SIZES;

    uintptr_t ptr = (uintptr_t)&(grf_chunk->data[0]);
    uintptr_t end = (uintptr_t)src + riff_size + 8;

    while (ptr < end)
    {
        const RIFFChunk *chunk = (const RIFFChunk *)ptr;

        uint32_t id = chunk->id;
        uint32_t size = chunk->size;
        const void *data = (const void *)&(chunk->data[0]);

        ptr += size + 8;

        GRFError ret = GRF_NO_ERROR;

        switch (id)
        {
            case ID_HDRX:
                if (size != sizeof(GRFHeader))
                    return GRF_INCONSISTENT_SIZES;
                if (header)
                    memcpy(header, data, size);
                break;
            case ID_GFX:
                if (gfxDst)
                    ret = grfExtract(data, gfxDst, gfxSize);
                break;
            case ID_MAP:
                if (mapDst)
                    ret = grfExtract(data, mapDst, mapSize);
                break;
            case ID_MTIL:
                if (mtilDst)
                    ret = grfExtract(data, mtilDst, mtilSize);
                break;
            case ID_MMAP:
                if (mmapDst)
                    ret = grfExtract(data, mmapDst, mmapSize);
                break;
            case ID_PAL:
                if (palDst)
                    ret = grfExtract(data, palDst, palSize);
                break;
            default:
                // Ignore unknown chunks rather than failing
                break;
        }

        if (ret != GRF_NO_ERROR)
            return ret;
    }

    return GRF_NO_ERROR;
}

GRFError grfLoadMem(const void *src, GRFHeader *header,
                    void **gfxDst, size_t *gfxSize,
                    void **mapDst, size_t *mapSize,
                    void **palDst, size_t *palSize)
{
    return grfLoadMemEx(src, header, gfxDst, gfxSize, mapDst, mapSize,
                        palDst, palSize, NULL, NULL, NULL, NULL);
}

// Extracts a GRF item from a FILE pointer
static GRFError grfExtractFile(FILE *file, size_t chunk_size,
                               void **dst, size_t *sz)
{
    if ((file == NULL) || (chunk_size == 0) || (dst == NULL))
        return GRF_NULL_POINTER;

    // The header of this data is the header used for all GBA/NDS BIOS
    // decompression routines. Uncompressed chunks also use the same format for
    // consistency.

    uint32_t header;

    if (fread(&header, sizeof(header), 1, file) != 1)
        return GRF_FILE_NOT_READ;

    uint32_t size = header >> 8;

    // Allocate destination buffer
    if (sz != NULL)
        *sz = size;

    // If the user has already provided a pointer, use it. If not, allocate mem
    if (*dst == NULL)
    {
        *dst = malloc(size);
        if (*dst == NULL)
            return GRF_NOT_ENOUGH_MEMORY;
    }

    if ((header & 0xF0) == 0x00)
    {
        // No compression. Read the data to the destination buffer
        if (fread(*dst, 1, chunk_size - 4, file) != (chunk_size - 4))
            return GRF_FILE_NOT_READ;

        return GRF_NO_ERROR;
    }

    // Allocate temporary buffer to hold the compressed contents of the file

    uint32_t *tmp = malloc(chunk_size);
    if (tmp == NULL)
        return GRF_NOT_ENOUGH_MEMORY;

    // We have already read the header. Read the rest of the chunk.
    *tmp = header;
    if (fread(tmp + 1, 1, chunk_size - 4, file) != (chunk_size - 4))
        return GRF_FILE_NOT_READ;

    GRFError err = GRF_NO_ERROR;

    switch (header & 0xF0)
    {
        case 0x10: // LZ77
            decompress(tmp, *dst, LZ77Vram);
            break;
        case 0x20: // Huffman
            decompress(tmp, *dst, HUFF);
            break;
        case 0x30: // RLE
            decompress(tmp, *dst, RLEVram);
            break;
        default:
            err = GRF_UNKNOWN_COMPRESSION;
            break;
    }

    // Free the temporary buffer
    free(tmp);

    return err;
}

GRFError grfLoadFileEx(FILE *file, GRFHeader *header,
                       void **gfxDst, size_t *gfxSize,
                       void **mapDst, size_t *mapSize,
                       void **palDst, size_t *palSize,
                       void **mtilDst, size_t *mtilSize,
                       void **mmapDst, size_t *mmapSize)
{
    if (file == NULL)
        return GRF_NULL_POINTER;

    RIFFChunk riff_chunk;

    if (fread(&riff_chunk, sizeof(RIFFChunk), 1, file) != 1)
        return GRF_FILE_NOT_READ;

    if (riff_chunk.id != ID_RIFF)
        return GRF_INVALID_ID_RIFF;

    RIFFChunk grf_chunk;

    if (fread(&grf_chunk, sizeof(RIFFChunk), 1, file) != 1)
        return GRF_FILE_NOT_READ;

    if (grf_chunk.id != ID_GRF)
        return GRF_INVALID_ID_GRF;

    // Ensure that both sizes are consistent
    if (riff_chunk.size != (grf_chunk.size + 8))
        return GRF_INCONSISTENT_SIZES;

    while (1)
    {
        RIFFChunk chunk;

        // Try to read the ID and size of a chunk. If it fails, it could be that
        // we have reached the end of the file. If so, exit without flagging an
        // error.
        if (fread(&chunk, sizeof(RIFFChunk), 1, file) != 1)
        {
            if (feof(file) != 0)
                break;
            else
                return GRF_FILE_NOT_READ;
        }

        uint32_t id = chunk.id;
        uint32_t size = chunk.size;

        GRFError ret = GRF_NO_ERROR;

        switch (id)
        {
            case ID_HDRX:
                if (size != sizeof(GRFHeader))
                    return GRF_INCONSISTENT_SIZES;

                if (header)
                {
                    if (fread(header, sizeof(GRFHeader), 1, file) != 1)
                        return GRF_FILE_NOT_READ;
                }
                else
                {
                    if (fseek(file, sizeof(GRFHeader), SEEK_CUR) != 0)
                        return GRF_FILE_NOT_READ;
                }
                break;

            case ID_GFX:
                if (gfxDst)
                {
                    ret = grfExtractFile(file, size, gfxDst, gfxSize);
                }
                else
                {
                    if (fseek(file, size, SEEK_CUR) != 0)
                        ret = GRF_FILE_NOT_READ;
                }
                break;

            case ID_MAP:
                if (mapDst)
                {
                    ret = grfExtractFile(file, size, mapDst, mapSize);
                }
                else
                {
                    if (fseek(file, size, SEEK_CUR) != 0)
                        ret = GRF_FILE_NOT_READ;
                }
                break;

            case ID_MTIL:
                if (mtilDst)
                {
                    ret = grfExtractFile(file, size, mtilDst, mtilSize);
                }
                else
                {
                    if (fseek(file, size, SEEK_CUR) != 0)
                        ret = GRF_FILE_NOT_READ;
                }
                break;

            case ID_MMAP:
                if (mmapDst)
                {
                    ret = grfExtractFile(file, size, mmapDst, mmapSize);
                }
                else
                {
                    if (fseek(file, size, SEEK_CUR) != 0)
                        ret = GRF_FILE_NOT_READ;
                }
                break;

            case ID_PAL:
                if (palDst)
                {
                    ret = grfExtractFile(file, size, palDst, palSize);
                }
                else
                {
                    if (fseek(file, size, SEEK_CUR) != 0)
                        ret = GRF_FILE_NOT_READ;
                }
                break;

            default:
                // Ignore unknown chunks rather than failing
                if (fseek(file, size, SEEK_CUR) != 0)
                    ret = GRF_FILE_NOT_READ;
                break;
        }

        if (ret != GRF_NO_ERROR)
            return ret;
    }

    return GRF_NO_ERROR;
}

GRFError grfLoadFile(FILE *file, GRFHeader *header,
                     void **gfxDst, size_t *gfxSize,
                     void **mapDst, size_t *mapSize,
                     void **palDst, size_t *palSize)
{
    return grfLoadFileEx(file, header, gfxDst, gfxSize, mapDst, mapSize,
                         palDst, palSize, NULL, NULL, NULL, NULL);
}

GRFError grfLoadPathEx(const char *path, GRFHeader *header,
                       void **gfxDst, size_t *gfxSize,
                       void **mapDst, size_t *mapSize,
                       void **palDst, size_t *palSize,
                       void **mtilDst, size_t *mtilSize,
                       void **mmapDst, size_t *mmapSize)
{
    if (path == NULL)
        return GRF_NULL_POINTER;

    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return GRF_FILE_NOT_OPENED;

    GRFError ret = grfLoadFileEx(file, header, gfxDst, gfxSize, mapDst, mapSize,
                                 palDst, palSize, mtilDst, mtilSize,
                                 mmapDst, mmapSize);

    if (fclose(file) != 0)
        return GRF_FILE_NOT_CLOSED;

    return ret;
}

GRFError grfLoadPath(const char *path, GRFHeader *header,
                     void **gfxDst, size_t *gfxSize,
                     void **mapDst, size_t *mapSize,
                     void **palDst, size_t *palSize)
{
    return grfLoadPathEx(path, header, gfxDst, gfxSize, mapDst, mapSize,
                         palDst, palSize, NULL, NULL, NULL, NULL);
}
