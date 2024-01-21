// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2024 Antonio Niño Díaz

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
/// Check https://www.coranac.com/man/grit/html/grit.htm for more information.

#include <stdint.h>

/// Header chunk of a GRF file.
typedef struct {
    uint8_t  gfxAttr;  ///< BPP of graphics. 0 if not present.
    uint8_t  mapAttr;  ///< BPP of map (16 or 8 for affine). 0 if not present.
    uint8_t  mmapAttr; ///< BPP of metamap (16). 0 if not present.
    uint8_t  palAttr;  ///< BPP of palette (16). 0 if not present.
    uint8_t  tileWidth, tileHeight; ///< Size of tiles in pixels
    uint8_t  metaWidth, metaHeight; ///< Size of metamap in tiles
    uint32_t gfxWidth, gfxHeight;   ///< Size of graphics in pixels
} grf_header_t;

/// From a GRF file in RAM, extract all data and allocate memory for it.
///
/// This function lets you decide which components of the GRF file have to be
/// loaded, and whether they have to be loaded to a hardcoded address or if the
/// function needs to allocate memory for them.
///
/// Note that Huffman decompression isn't VRAM-safe. RLE and LZ77 are VRAM-safe.
/// If using Huffman compression with your GRF files, don't hardcode the
/// destination address to VRAM.
///
/// Let function allocate memory:
/// ```
/// void *gfxDst = NULL;
/// int ret = grfLoad(grf_file, NULL, &gfxDst, NULL, NULL, NULL, NULL);
/// if (ret == 0)
/// {
///     // Use data here...
/// }
/// free(gfxDst);
/// ```
///
/// Hardcode destination address:
/// ```
/// uint16_t palette[256];
/// void *palDst = (void *)&palette[0]; // Load data to the array
/// int ret = grfLoad(grf_file, NULL, NULL, NULL, &palDst, NULL, NULL);
/// if (ret == 0)
/// {
///     // Use data here...
/// }
/// ```
///
/// Example of reading the header:
/// ```
/// grf_header_t header = {0};
/// int ret = grfLoad(grf_file, &header, NULL, NULL, NULL, NULL, NULL);
/// if (ret == 0)
/// {
///     // Use data here...
/// }
/// ```
///
/// @param src Pointer to the GRF file in RAM.
/// @param header Pointer to a header structure to be filled.
/// @param gfxDst Pointer to pointer to load graphics data.
/// @param mapDst Pointer to pointer to load map data.
/// @param palDst Pointer to pointer to load palette data.
/// @param mtilDst Pointer to pointer to load metatile data.
/// @param mmapDst Pointer to pointer to load metamap data.
///
/// @return Returns 0 on success, a negative number on error.
int grfLoad(const void *src, grf_header_t *header, void **gfxDst,
            void **mapDst, void **palDst, void **mtilDst, void **mmapDst);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_GRF_H__
