// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/ndstypes.h>
#include <nds/arm9/image.h>
#include <nds/arm9/sassert.h>
#include <nds/dma.h>

#include <string.h>
#include <malloc.h>

void image24to16(sImage* img)
{
	int x;
	int y;

	u16* temp = (u16*)malloc(img->height*img->width*2);

	for(y=0;y<img->height;y++)
	{
		for(x=0;x<img->width;x++)
			temp[x+y*img->width]=(1<<15)|RGB15(img->image.data8[x*3+y*img->width*3]>>3, \
			img->image.data8[x*3+y*img->width*3+1]>>3, img->image.data8[x*3+y*img->width*3+2]>>3);
	}

	free(img->image.data8);

	img->bpp=16;
	img->image.data16 = temp;
}

void image8to16(sImage* img)
{
	int i;

	sassert(img->bpp == 8, "image must be 8 bpp");
	sassert(img->palette != NULL, "image must have a palette set");

	u16* temp = (u16*)malloc(img->height*img->width*2);

	for(i = 0; i < img->height * img->width; i++)
		temp[i] = img->palette[img->image.data8[i]] | (1<<15);

	free (img->image.data8);
	free (img->palette);

	img->palette = NULL;

	img->bpp = 16;
	img->image.data16 = temp;
}

void image8to16trans(sImage* img, u8 transparentColor)
{
	int i;
	u8 c;

	sassert(img->bpp == 8, "image must be 8 bpp");
	sassert(img->palette != NULL, "image must have a palette set");

	u16* temp = (u16*)malloc(img->height*img->width*2);

	for(i = 0; i < img->height * img->width; i++) {

		c = img->image.data8[i];

		if(c != transparentColor)
			temp[i] = img->palette[c] | (1<<15);
		else
			temp[i] = img->palette[c];
	}

	free (img->image.data8);
	free (img->palette);

	img->palette = NULL;

	img->bpp = 16;
	img->image.data16 = temp;
}

void imageTileData(sImage* img)
{
	u32* temp;
	
	int ix, iy, tx, ty;

	int th, tw;

	int i = 0;
	
	//can only tile 8 bit data that is a multiple of 8 in dimention
	if(img->bpp != 8 || (img->height & 3) != 0 || (img->width & 3) != 0) return;

	th = img->height >> 3;
	tw = img->width >> 3;

	//buffer to hold data
	temp = (u32*)malloc(img->height * img->width);	

	for(ty = 0; ty < th; ty++)
		for(tx = 0; tx < tw; tx++)
			for(iy = 0; iy < 8; iy++)
				for(ix = 0; ix < 2; ix++)
					temp[i++] = img->image.data32[ix + tx * 2 + (iy + ty * 8) * tw * 2 ]; 

	free(img->image.data32);
	
	img->image.data32 = (u32*)temp;
}

void imageDestroy(sImage* img)
{
	if(img->image.data8) free (img->image.data8);
	if(img->palette && img->bpp == 8) free (img->palette);
}
