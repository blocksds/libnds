// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/arm9/pcx.h
///
/// @brief A simple 256 color pcx file loader.

#ifndef LIBNDS_NDS_ARM9_PCX_H__
#define LIBNDS_NDS_ARM9_PCX_H__

typedef struct PCXHeader
{
   char      manufacturer;  // Should be 0
   char      version;       // Should be 5
   char      encoding;      // Should be 1
   char      bitsPerPixel;  // Should be 8
   short int xmin,ymin;     // Coordinates for top left,bottom right
   short int xmax,ymax;
   short int hres;          // Resolution
   short int vres;
   char      palette16[48]; // 16 color palette if 16 color image
   char      reserved;      // Ignore
   char      colorPlanes;   // Ignore
   short int bytesPerLine;
   short int paletteYype;   // Should be 2
   char      filler[58];    // Ignore
}__attribute__ ((packed)) PCXHeader, *pPCXHeader;

#ifdef __cplusplus
extern "C" {
#endif

/// Loads an image structure with data from PCX formatted data.
///
/// @param pcx A pointer to the pcx file loaded into memory.
/// @param image The image structure to fill in (the loader will allocate room
///              for the palette and pixel data)
/// @return true on success, false on failure.
bool loadPCX(const unsigned char *pcx, sImage *image);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_PCX_H__
