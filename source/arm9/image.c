// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <stdlib.h>
#include <string.h>

#include <nds/arm9/image.h>
#include <nds/arm9/sassert.h>
#include <nds/dma.h>
#include <nds/ndstypes.h>

bool image24to16(sImage *img)
{
    u16 *temp = malloc(img->height * img->width * sizeof(u16));
    if (temp == NULL)
        return false;

    for (int y = 0; y < img->height; y++)
    {
        for (int x = 0; x < img->width; x++)
        {
            temp[x + y * img->width] =
                (1 << 15)
                | RGB15(img->image.data8[x * 3 + y * img->width * 3] >> 3,
                        img->image.data8[x * 3 + y * img->width * 3 + 1] >> 3,
                        img->image.data8[x * 3 + y * img->width * 3 + 2] >> 3);
        }
    }

    free(img->image.data8);

    img->bpp = 16;
    img->image.data16 = temp;

    return true;
}

bool image8to16(sImage *img)
{
    sassert(img->bpp == 8, "image must be 8 bpp");
    sassert(img->palette != NULL, "image must have a palette set");

    u16 *temp = malloc(img->height * img->width * sizeof(u16));
    if (temp == NULL)
        return false;

    for (int i = 0; i < img->height * img->width; i++)
        temp[i] = img->palette[img->image.data8[i]] | (1 << 15);

    free(img->image.data8);
    free(img->palette);

    img->palette = NULL;

    img->bpp = 16;
    img->image.data16 = temp;

    return true;
}

bool image8to16trans(sImage *img, u8 transparentColor)
{
    sassert(img->bpp == 8, "image must be 8 bpp");
    sassert(img->palette != NULL, "image must have a palette set");

    u16 *temp = malloc(img->height * img->width * sizeof(u16));
    if (temp == NULL)
        return false;

    for (int i = 0; i < img->height * img->width; i++)
    {
        u8 c = img->image.data8[i];

        if (c != transparentColor)
            temp[i] = img->palette[c] | (1 << 15);
        else
            temp[i] = img->palette[c];
    }

    free(img->image.data8);
    free(img->palette);

    img->palette = NULL;

    img->bpp = 16;
    img->image.data16 = temp;

    return true;
}

bool imageTileData(sImage *img)
{
    // Can only tile 8 bit data that is a multiple of 8 in dimention
    sassert(img->bpp == 8, "image must be 8 bpp");
    sassert((img->height & 7) == 0 && (img->width & 7) == 0, "image must be a multiple of 8 in dimension");

    int th = img->height >> 3;
    int tw = img->width >> 3;

    // Buffer to hold data
    u32 *temp = malloc(img->height * img->width * sizeof(u32));
    if (temp == NULL)
        return false;

    int i = 0;

    for (int ty = 0; ty < th; ty++)
    {
        for (int tx = 0; tx < tw; tx++)
        {
            for (int iy = 0; iy < 8; iy++)
            {
                for (int ix = 0; ix < 2; ix++)
                    temp[i++] = img->image.data32[ix + tx * 2 + (iy + ty * 8) * tw * 2];
            }
        }
    }

    free(img->image.data32);

    img->image.data32 = (u32 *)temp;

    return true;
}

void imageDestroy(sImage *img)
{
    if (img->image.data8)
        free(img->image.data8);

    if (img->palette && img->bpp == 8)
        free(img->palette);
}
