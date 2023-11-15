// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <stdlib.h>

#include <nds/arm9/image.h>
#include <nds/arm9/pcx.h>
#include <nds/arm9/video.h>

bool loadPCX(const unsigned char *pcx, sImage *image)
{
    // struct rgb {
    //     unsigned char b,g,r;
    // };

    RGB_24 *pal;

    PCXHeader *hdr = (PCXHeader *)pcx;

    pcx += sizeof(PCXHeader);

    int scansize = hdr->bytesPerLine;

    int width = image->width = hdr->xmax - hdr->xmin + 1;
    int height = image->height = hdr->ymax - hdr->ymin + 1;

    int size = image->width * image->height;

    if (hdr->bitsPerPixel != 8)
        return false;

    unsigned char *scanline = image->image.data8 = malloc(size);
    if (scanline == NULL)
        return false;

    image->palette = malloc(256 * 2);
    if (image->palette == NULL)
    {
        free(scanline);
        return false;
    }

    int count = 0;

    for (int iy = 0; iy < height; iy++)
    {
        count = 0;
        while (count < scansize)
        {
            unsigned char c = *pcx++;

            if (c < 192)
            {
                scanline[count++] = c;
            }
            else
            {
                int run = c - 192;
                c = *pcx++;

                for (int i = 0; i < run && count < scansize; i++)
                    scanline[count++] = c;
            }
        }
        scanline += width;
    }

    // check for the palette marker.
    // I have seen PCX files without this, but the docs don't seem ambiguous--it must be
    // here. Anyway, the support among other apps is poor, so we're going to reject it.
    if (*pcx != 0x0C)
    {
        free(image->image.data8);
        image->image.data8 = 0;
        free(image->palette);
        image->palette = 0;
        return false;
    }

    pcx++;

    pal = (RGB_24 *)(pcx);

    image->bpp = 8;

    for (int i = 0; i < 256; i++)
    {
        u8 r = (pal[i].r + 4 > 255) ? 255 : (pal[i].r + 4);
        u8 g = (pal[i].g + 4 > 255) ? 255 : (pal[i].g + 4);
        u8 b = (pal[i].b + 4 > 255) ? 255 : (pal[i].b + 4);
        image->palette[i] = RGB15(r >> 3, g >> 3, b >> 3);
    }

    return true;
}
