// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/arm9/image.h
///
/// @brief An image abstraction for working with image data.
///
/// Image data buffers must be allocated using malloc() rather than pointing to
/// stack data, as the conversion routines will free() the argument's image
/// buffer and allocate a new block for the replacement data.
///
/// As such, any loader implemented utilizing this structure must use malloc()
/// to allocate the image buffer.

#ifndef LIBNDS_NDS_ARM9_IMAGE_H__
#define LIBNDS_NDS_ARM9_IMAGE_H__

#include <nds/arm9/video.h>

/// Holds a red green blue triplet
typedef struct RGB_24
{
    unsigned char r; ///< 8 bits for the red value.
    unsigned char g; ///< 8 bits for the green value.
    unsigned char b; ///< 8 bits for the blue value.
} __attribute__ ((packed)) RGB_24;

/// A generic image structure.
typedef struct sImage
{
    short height;            ///< \brief The height of the image in pixels
    short width;             ///< \brief The width of the image in pixels
    int bpp;                 ///< \brief Bits per pixel (should be 4 8 16 or 24)
    unsigned short *palette; ///< \brief A pointer to the palette data

    /// A union of data pointers to the pixel data.
    union
    {
        u8 *data8;   ///< pointer to 8 bit data.
        u16 *data16; ///< pointer to 16 bit data.
        u32 *data32; ///< pointer to 32 bit data.
    } image;

} sImage, *psImage;

#ifdef __cplusplus
extern "C" {
#endif

/// Destructively converts a 24-bit image to 16-bit
///
/// @param img Pointer to the image to manipulate.
/// @return true on success, false on failure.
bool image24to16(sImage *img);

/// Destructively converts an 8-bit image to 16 bit setting the alpha bit.
///
/// @param img Pointer to the image to manipulate.
/// @return true on success, false on failure.
bool image8to16(sImage *img);

/// Destructively converts an 8-bit image to 16-bit with alpha bit cleared for
/// the supplied palette index.
///
/// @param img Pointer to the image to manipulate.
/// @param transparentColor Color indexes equal to this value will have the
///                         alpha bit clear
/// @return true on success, false on failure.
bool image8to16trans(sImage *img, u8 transparentColor);

/// Frees the image data.
///
/// Only call if the image data was returned from an image loader.
///
/// @param img Pointer to the image to manipulate.
void imageDestroy(sImage *img);

/// Tiles 8-bit image data into a sequence of 8x8 tiles.
///
/// @param img Pointer to the image to manipulate.
/// @return true on success, false on failure.
bool imageTileData(sImage *img);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_IMAGE_H__
