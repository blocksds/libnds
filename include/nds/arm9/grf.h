// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2024 Antonio Niño Díaz

#ifndef LIBNDS_NDS_ARM9_GRF_H__
#define LIBNDS_NDS_ARM9_GRF_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/arm9/grf.h
///
/// @brief Functions to load GRF files.
///
/// This is one of the formats that GRIT can generate, and it's useful because
/// it packs multiple graphics blobs as well as metadata such as compression
/// type and size.
///
/// GRF files may contain compressed and uncompressed data blobs in the same
/// file. Compressed blobs may use different compression algorithms. Check the
/// documentation of decompress() for more information about the supported
/// formats. Note that all compression formats supported by grit are also
/// supported by decompress().
///
/// Check https://www.coranac.com/man/grit/html/grit.htm for more information.

#include <stdint.h>
#include <stdio.h>

/// Header chunk of a GRF file.
typedef struct
{
    uint16_t version;  ///< Version of the GRF format (currently 2)
    uint16_t gfxAttr;  ///< BPP of graphics (or GRFTextureTypes). 0 if not present.
    uint16_t mapAttr;  ///< BPP of map (16 or 8 for affine). 0 if not present.
    uint16_t mmapAttr; ///< BPP of metamap (16). 0 if not present.
    uint16_t palAttr;  ///< Number of colors of the palette. 0 if not present.
    uint8_t  tileWidth, tileHeight; ///< Size of tiles in pixels
    uint8_t  metaWidth, metaHeight; ///< Size of metamap in tiles
    uint16_t unused; ///< Currently unused
    uint32_t gfxWidth, gfxHeight;   ///< Size of graphics in pixels
} GRFHeader;

/// Special values for the GFX attribute field for NDS textures.
typedef enum
{
    GRF_TEXFMT_A5I3 = 128,
    GRF_TEXFMT_A3I5 = 129,
    GRF_TEXFMT_4x4  = 130,
} GRFTextureTypes;

/// Special values for the MAP attribute field that define background types.
typedef enum
{
    GRF_BGFMT_NO_DATA        = 0, ///< No map data present
    GRF_BGFMT_REG_16x16      = 1, ///< Regular, 16 palettes of 16 colors
    GRF_BGFMT_REG_256x1      = 2, ///< Regular, 1 palette of 256 colors
    GRF_BGFMT_AFF_256x1      = 3, ///< Affine, 1 palette of 256 colors
    GRF_BGFMT_AFF_EXT_256x16 = 4, ///< Extended affine, 16 palettes of 256 colors
} GRFMapType;

/// Possible errors that can happen while reading GRF files.
typedef enum
{
    GRF_NO_ERROR                = 0,  ///< No error happened
    GRF_NULL_POINTER            = -1, ///< NULL pointer passed as argument
    GRF_FILE_NOT_OPENED         = -2, ///< Failed to open file with fopen()
    GRF_FILE_NOT_READ           = -3, ///< Failed to read file
    GRF_FILE_NOT_CLOSED         = -4, ///< Failed to close file with fclose()
    GRF_INVALID_ID_RIFF         = -5, ///< Chunk ID "RIFF" not found
    GRF_INVALID_ID_GRF          = -6, ///< Chunk ID "GRF " not found
    GRF_INCONSISTENT_SIZES      = -7, ///< The size of a chunk is invalid
    GRF_NOT_ENOUGH_MEMORY       = -8, ///< Not enough memory for malloc()
    GRF_UNKNOWN_COMPRESSION     = -9, ///< Unknown graphics compression format
} GRFError;

/// From a GRF file in RAM, extract all data and allocate memory for it.
///
/// This function lets you decide which components of the GRF file have to be
/// loaded and whether they have to be loaded to a hardcoded address or if the
/// function needs to allocate memory for them. Values that aren't needed can be
/// ignored by passing NULL to the specific argument of the function.
///
/// Note that Huffman decompression isn't VRAM-safe. RLE and LZ77 are VRAM-safe.
/// If using Huffman compression with your GRF files, don't hardcode the
/// destination address to VRAM.
///
/// Let function allocate memory and inform you of the size of the buffer:
/// ```
/// void *gfxDst = NULL;
/// size_t gfxSize;
/// GRFError ret = grfLoadMemEx(grf_file, NULL, &gfxDst, &gfxSize, NULL, NULL,
///                             NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
/// if (ret == GRF_NO_ERROR)
/// {
///     // Use data here...
/// }
/// free(gfxDst);
/// ```
///
/// Hardcode destination address, ignore resulting size:
/// ```
/// uint16_t palette[256];
/// void *palDst = (void *)&palette[0]; // Load data to the array
/// GRFError ret = grfLoadMemEx(grf_file, NULL, NULL, NULL, NULL, NULL,
///                             &palDst, NULL, NULL, NULL, NULL, NULL);
/// if (ret == GRF_NO_ERROR)
/// {
///     // Use data here...
/// }
/// ```
///
/// Example of reading the header:
/// ```
/// GRFHeader header = {0};
/// GRFError ret = grfLoadMemEx(grf_file, &header, NULL, NULL, NULL, NULL,
///                             NULL, NULL, NULL, NULL, NULL, NULL);
/// if (ret == GRF_NO_ERROR)
/// {
///     // Use data here...
/// }
/// ```
///
/// @param src
///     Pointer to the GRF file in RAM.
/// @param header
///     Pointer to a header structure to be filled.
/// @param gfxDst
///     Pointer to pointer to load graphics data.
/// @param gfxSize
///     Location to store the graphics data size.
/// @param mapDst
///     Pointer to pointer to load map data.
/// @param mapSize
///     Location to store the map data size.
/// @param palDst
///     Pointer to pointer to load palette data.
/// @param palSize
///     Location to store the palette data size.
/// @param mtilDst
///     Pointer to pointer to load metatile data.
/// @param mtilSize
///     Location to store the metatile data size.
/// @param mmapDst
///     Pointer to pointer to load metamap data.
/// @param mmapSize
///     Location to store the metamap data size.
///
/// @return
///     Returns 0 on success, a negative number on error.
GRFError grfLoadMemEx(const void *src, GRFHeader *header,
                      void **gfxDst, size_t *gfxSize,
                      void **mapDst, size_t *mapSize,
                      void **palDst, size_t *palSize,
                      void **mtilDst, size_t *mtilSize,
                      void **mmapDst, size_t *mmapSize);

/// From a GRF file in RAM, extract all data and allocate memory for it.
///
/// @note
///     Check grfLoadMemEx() for details about how to use this function.
///
/// @param src
///     Pointer to the GRF file in RAM.
/// @param header
///     Pointer to a header structure to be filled.
/// @param gfxDst
///     Pointer to pointer to load graphics data.
/// @param gfxSize
///     Location to store the graphics data size.
/// @param mapDst
///     Pointer to pointer to load map data.
/// @param mapSize
///     Location to store the map data size.
/// @param palDst
///     Pointer to pointer to load palette data.
/// @param palSize
///     Location to store the palette data size.
///
/// @return
///     Returns 0 on success, a negative number on error.
GRFError grfLoadMem(const void *src, GRFHeader *header,
                    void **gfxDst, size_t *gfxSize,
                    void **mapDst, size_t *mapSize,
                    void **palDst, size_t *palSize);

/// From a FILE* to a GRF file, extract all data and allocate memory for it.
///
/// @note
///     Check grfLoadMemEx() for details about how to use this function.
///
/// @param file
///     FILE handle to the GRF file.
/// @param header
///     Pointer to a header structure to be filled.
/// @param gfxDst
///     Pointer to pointer to load graphics data.
/// @param gfxSize
///     Location to store the graphics data size.
/// @param mapDst
///     Pointer to pointer to load map data.
/// @param mapSize
///     Location to store the map data size.
/// @param palDst
///     Pointer to pointer to load palette data.
/// @param palSize
///     Location to store the palette data size.
///
/// @return
///     Returns 0 on success, a negative number on error.
GRFError grfLoadFile(FILE *file, GRFHeader *header,
                     void **gfxDst, size_t *gfxSize,
                     void **mapDst, size_t *mapSize,
                     void **palDst, size_t *palSize);

/// From a FILE* to a GRF file, extract all data and allocate memory for it.
///
/// @note
///     Check grfLoadMemEx() for details about how to use this function.
///
/// @param file
///     FILE handle to the GRF file.
/// @param header
///     Pointer to a header structure to be filled.
/// @param gfxDst
///     Pointer to pointer to load graphics data.
/// @param gfxSize
///     Location to store the graphics data size.
/// @param mapDst
///     Pointer to pointer to load map data.
/// @param mapSize
///     Location to store the map data size.
/// @param palDst
///     Pointer to pointer to load palette data.
/// @param palSize
///     Location to store the palette data size.
/// @param mtilDst
///     Pointer to pointer to load metatile data.
/// @param mtilSize
///     Location to store the metatile data size.
/// @param mmapDst
///     Pointer to pointer to load metamap data.
/// @param mmapSize
///     Location to store the metamap data size.
///
/// @return
///     Returns 0 on success, a negative number on error.
GRFError grfLoadFileEx(FILE *file, GRFHeader *header,
                       void **gfxDst, size_t *gfxSize,
                       void **mapDst, size_t *mapSize,
                       void **palDst, size_t *palSize,
                       void **mtilDst, size_t *mtilSize,
                       void **mmapDst, size_t *mmapSize);

/// From a path to a GRF file, extract all data and allocate memory for it.
///
/// @note Check grfLoadMemEx() for details about how to use this function.
///
/// @param path
///     Path to the GRF file in the filesystem.
/// @param header
///     Pointer to a header structure to be filled.
/// @param gfxDst
///     Pointer to pointer to load graphics data.
/// @param gfxSize
///     Location to store the graphics data size.
/// @param mapDst
///     Pointer to pointer to load map data.
/// @param mapSize
///     Location to store the map data size.
/// @param palDst
///     Pointer to pointer to load palette data.
/// @param palSize
///     Location to store the palette data size.
///
/// @return
///     Returns 0 on success, a negative number on error.
GRFError grfLoadPath(const char *path, GRFHeader *header,
                     void **gfxDst, size_t *gfxSize,
                     void **mapDst, size_t *mapSize,
                     void **palDst, size_t *palSize);

/// From a path to a GRF file, extract all data and allocate memory for it.
///
/// @note Check grfLoadMemEx() for details about how to use this function.
///
/// @param path
///     Path to the GRF file in the filesystem.
/// @param header
///     Pointer to a header structure to be filled.
/// @param gfxDst
///     Pointer to pointer to load graphics data.
/// @param gfxSize
///     Location to store the graphics data size.
/// @param mapDst
///     Pointer to pointer to load map data.
/// @param mapSize
///     Location to store the map data size.
/// @param palDst
///     Pointer to pointer to load palette data.
/// @param palSize
///     Location to store the palette data size.
/// @param mtilDst
///     Pointer to pointer to load metatile data.
/// @param mtilSize
///     Location to store the metatile data size.
/// @param mmapDst
///     Pointer to pointer to load metamap data.
/// @param mmapSize
///     Location to store the metamap data size.
///
/// @return
///     Returns 0 on success, a negative number on error.
GRFError grfLoadPathEx(const char *path, GRFHeader *header,
                       void **gfxDst, size_t *gfxSize,
                       void **mapDst, size_t *mapSize,
                       void **palDst, size_t *palSize,
                       void **mtilDst, size_t *mtilSize,
                       void **mmapDst, size_t *mmapSize);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_GRF_H__
