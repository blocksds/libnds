// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007 Dave Murphy (WinterMute)
// Copyright (C) 2007 Jason Rogers (dovoto)

// DS Background Control

#include <string.h>

#include <nds/arm9/background.h>
#include <nds/arm9/trig_lut.h>

// Look up tables for smoothing register access between the two displays
vu16 *const bgControl[8] =
{
    &REG_BG0CNT,
    &REG_BG1CNT,
    &REG_BG2CNT,
    &REG_BG3CNT,
    &REG_BG0CNT_SUB,
    &REG_BG1CNT_SUB,
    &REG_BG2CNT_SUB,
    &REG_BG3CNT_SUB,
};

bg_scroll *const bgScrollTable[8] =
{
    &BG_OFFSET[0],
    &BG_OFFSET[1],
    &BG_OFFSET[2],
    &BG_OFFSET[3],

    &BG_OFFSET_SUB[0],
    &BG_OFFSET_SUB[1],
    &BG_OFFSET_SUB[2],
    &BG_OFFSET_SUB[3]
};

bg_transform *const bgTransform[8] =
{
    (bg_transform *)0,
    (bg_transform *)0,
    (bg_transform *)0x04000020,
    (bg_transform *)0x04000030,

    (bg_transform *)0,
    (bg_transform *)0,
    (bg_transform *)0x04001020,
    (bg_transform *)0x04001030,
};

BgState bgState[8];

bool bgIsTextLut[8];

bool bgIsText(int id)
{
    return bgIsTextLut[id];
}

bool checkIfText(int id)
{
    if (id < 2 || (id > 3 && id < 6))
        return true;

    u8 mode = (id < 4) ? (videoGetMode() & 7) : (videoGetModeSub() & 7);

    if (mode == 0)
        return true;

    if (mode == 1 || mode == 3)
    {
        return id == 3 || id == 7 ? false : true;
    }

    return false;
}

void bgUpdate(void)
{
    for (int i = 0; i < 8; i++)
    {
        if (!bgState[i].dirty)
            continue;

        if (bgIsTextLut[i])
        {
            bgScrollTable[i]->x = bgState[i].scrollX >> 8;
            bgScrollTable[i]->y = bgState[i].scrollY >> 8;
        }
        else
        {
            s16 angleSin;
            s16 angleCos;

            s32 pa, pb, pc, pd;

            // Compute sin and cos
            angleSin = sinLerp(bgState[i].angle);
            angleCos = cosLerp(bgState[i].angle);

            // Set the background registers
            pa = (angleCos * bgState[i].scaleX) >> 12;
            pb = (-angleSin * bgState[i].scaleX) >> 12;
            pc = (angleSin * bgState[i].scaleY) >> 12;
            pd = (angleCos * bgState[i].scaleY) >> 12;

            bgTransform[i]->hdx = pa;
            bgTransform[i]->vdx = pb;
            bgTransform[i]->hdy = pc;
            bgTransform[i]->vdy = pd;

            bgTransform[i]->dx =
                bgState[i].scrollX
                - ((pa * bgState[i].centerX + pb * bgState[i].centerY) >> 8);
            bgTransform[i]->dy =
                bgState[i].scrollY
                - ((pc * bgState[i].centerX + pd * bgState[i].centerY) >> 8);
        }

        bgState[i].dirty = false;
    }
}

void bgSetRotate(int id, int angle)
{
    bgState[id].angle = angle;
    bgState[id].dirty = true;
}

void bgRotate(int id, int angle)
{
    sassert(!bgIsText(id), "Cannot Rotate a Text Background");

    bgSetRotate(id, angle + bgState[id].angle);
}

void bgSet(int id, int angle, s32 sx, s32 sy, s32 scrollX, s32 scrollY,
           s32 rotCenterX, s32 rotCenterY)
{
    bgState[id].scaleX = sx;
    bgState[id].scaleY = sy;

    bgState[id].scrollX = scrollX;
    bgState[id].scrollY = scrollY;

    bgState[id].centerX = rotCenterX;
    bgState[id].centerY = rotCenterY;

    bgState[id].angle = angle;

    bgState[id].dirty = true;
}

void bgSetRotateScale(int id, int angle, s32 sx, s32 sy)
{
    bgState[id].scaleX = sx;
    bgState[id].scaleY = sy;
    bgState[id].angle = angle;

    bgState[id].dirty = true;
}

void bgSetScale(int id, s32 sx, s32 sy)
{
    sassert(!bgIsText(id), "Cannot Scale a Text Background");

    bgState[id].scaleX = sx;
    bgState[id].scaleY = sy;

    bgState[id].dirty = true;
}

#ifndef NDEBUG
static void bgInitValidate(unsigned int videoMode, unsigned int layer, BgType type,
                           BgSize size, unsigned int mapBase, unsigned int tileBase)
{
    if (tileBase > 15)
        sassert(0, "BG tile base out of range");
    if (mapBase > 31)
        sassert(0, "BG map base out of range");

    if (type == BgType_Text8bpp || type == BgType_Text4bpp)
    {
        if (size != BgSize_T_256x256 && size != BgSize_T_512x256 &&
            size != BgSize_T_256x512 && size != BgSize_T_512x512)
            sassert(0, "Invalid type and size values");
    }
    else if (type == BgType_Rotation)
    {
        if (size != BgSize_R_128x128 && size != BgSize_R_256x256 &&
            size != BgSize_R_512x512 && size != BgSize_R_1024x1024)
            sassert(0, "Invalid type and size values");
    }
    else if (type == BgType_ExRotation)
    {
        if (size != BgSize_ER_128x128 && size != BgSize_ER_256x256 &&
            size != BgSize_ER_512x512 && size != BgSize_ER_1024x1024)
            sassert(0, "Invalid type and size values");
    }
    else if (type == BgType_Bmp8)
    {
        if (size != BgSize_B8_128x128 && size != BgSize_B8_256x256 &&
            size != BgSize_B8_512x256 && size != BgSize_B8_512x512 &&
            size != BgSize_B8_1024x512 && size != BgSize_B8_512x1024)
            sassert(0, "Invalid type and size values");

        if (tileBase > 0)
            sassert(0, "Tile base is unused for bitmaps");
    }
    else if (type == BgType_Bmp16)
    {
        if (size != BgSize_B16_128x128 && size != BgSize_B16_256x256 &&
            size != BgSize_B16_512x256 && size != BgSize_B16_512x512)
            sassert(0, "Invalid type and size values");

        if (tileBase > 0)
            sassert(0, "Tile base is unused for bitmaps");
    }

    if (layer == 0 || layer == 1)
    {
        if (videoMode == 6)
            sassert(0, "Layer not available in mode 6");

        // Layers 0 and 1 can only be text backgrounds
        if (type != BgType_Text8bpp && type != BgType_Text4bpp)
            sassert(0, "Incorrect background type for mode");
    }
    else if (layer == 2)
    {
        if (videoMode == 0 || videoMode == 1 || videoMode == 3)
        {
            if (type != BgType_Text8bpp && type != BgType_Text4bpp)
                sassert(0, "Incorrect background type for mode");
        }
        else if (videoMode == 2 || videoMode == 4)
        {
            if (type != BgType_Rotation)
                sassert(0, "Incorrect background type for mode");
        }
        else // if (videoMode == 5)
        {
            if (type != BgType_ExRotation && type != BgType_Bmp8 && type != BgType_Bmp16)
                sassert(0, "Incorrect background type for mode");
        }
    }
    else if (layer == 3)
    {
        if (videoMode == 0)
        {
            if (type != BgType_Text8bpp && type != BgType_Text4bpp)
                sassert(0, "Incorrect background type for mode");
        }
        else if (videoMode == 1 || videoMode == 2)
        {
            if (type != BgType_Rotation)
                sassert(0, "Incorrect background type for mode");
        }
        else if (videoMode == 3 || videoMode == 4 || videoMode == 5)
        {
            if (type != BgType_ExRotation && type != BgType_Bmp8 && type != BgType_Bmp16)
                sassert(0, "Incorrect background type for mode");
        }
        else // if (videoMode == 6)
        {
            sassert(0, "Layer not available in mode 6");
        }
    }
    else
    {
        sassert(0, "Layer out of range");
    }
}
#endif // NDEBUG

int bgInitHidden(int layer, BgType type, BgSize size, int mapBase, int tileBase)
{
    // Disable layer while it's being setup
    videoBgDisable(layer);

#ifndef NDEBUG
    unsigned int videoMode = videoGetMode();

    if ((videoMode & DISPLAY_MODE_MASK) != DISPLAY_MODE_NORMAL)
        sassert(0, "BGs only supported in normal display mode");

    videoMode &= 7; // Keep only the numeric mode

    if (videoMode == 7)
        sassert(0, "Invalid video mode 7");

    // Layer 0 can't be used for anything else if 3D is enabled
    if (layer == 0 && video3DEnabled())
        sassert(0, "Layer 0 is being used for 3D");

    bgInitValidate(videoMode, layer, type, size, mapBase, tileBase);

    if (size == BgSize_B8_512x1024 || size == BgSize_B8_1024x512)
    {
        if (videoMode != 6)
            sassert(0, "Large BMPs only supported in mode 6");

        if (mapBase > 0)
            sassert(0, "Large BMPs cannot be offset");
    }
#endif

    BGCTRL[layer] = BG_MAP_BASE(mapBase) | BG_TILE_BASE(tileBase) | size
                    | ((type == BgType_Text8bpp) ? BG_COLOR_256 : 0);

    memset(&bgState[layer], 0, sizeof(BgState));

    bgIsTextLut[layer] = checkIfText(layer);

    if (type != BgType_Text8bpp && type != BgType_Text4bpp)
    {
        bgSetScale(layer, 1 << 8, 1 << 8);
        bgSetRotate(layer, 0);
    }

    bgState[layer].type = type;
    bgState[layer].size = size;

    bgState[layer].dirty = true;

    bgUpdate();

    return layer;
}

int bgInit(int layer, BgType type, BgSize size, int mapBase, int tileBase)
{
    int ret = bgInitHidden(layer, type, size, mapBase, tileBase);
    bgShow(ret);
    return ret;
}

int bgInitHiddenSub(int layer, BgType type, BgSize size, int mapBase, int tileBase)
{
    // Disable layer while it's being setup
    videoBgDisableSub(layer);

#ifndef NDEBUG
    unsigned int videoMode = videoGetModeSub();

    if ((videoMode & DISPLAY_MODE_MASK) != DISPLAY_MODE_NORMAL)
        sassert(0, "BGs only supported in normal display mode");

    videoMode &= 7; // Keep only the numeric mode

    if (videoMode > 5)
        sassert(0, "Invalid sub video mode");

    if (size == BgSize_B8_512x1024 || size == BgSize_B8_1024x512)
        sassert(0, "Large BMPs only supported in main engine");

    bgInitValidate(videoMode, layer, type, size, mapBase, tileBase);
#endif

    BGCTRL_SUB[layer] = BG_MAP_BASE(mapBase) | BG_TILE_BASE(tileBase) | size
                        | ((type == BgType_Text8bpp) ? BG_COLOR_256 : 0);

    memset(&bgState[layer + 4], 0, sizeof(BgState));

    bgIsTextLut[layer + 4] = checkIfText(layer + 4);

    if (type != BgType_Text8bpp && type != BgType_Text4bpp)
    {
        bgSetScale(layer + 4, 1 << 8, 1 << 8);
        bgSetRotate(layer + 4, 0);
    }

    bgState[layer + 4].type = type;
    bgState[layer + 4].size = size;

    bgState[layer + 4].dirty = true;

    bgUpdate();

    return layer + 4;
}

int bgInitSub(int layer, BgType type, BgSize size, int mapBase, int tileBase)
{
    int ret = bgInitHiddenSub(layer, type, size, mapBase, tileBase);
    bgShow(ret);
    return ret;
}

vuint16 *bgSetControlBits(int id, u16 bits)
{
    sassert(id >= 0 && id <= 7,
            "bgSetControlBits(), id must be the number returned from bgInit or bgInitSub");
    *bgControl[id] |= bits;
    return bgControl[id];
}

void bgClearControlBits(int id, u16 bits)
{
    sassert(id >= 0 && id <= 7,
            "bgClearControlBits(), id must be the number returned from bgInit or bgInitSub");
    *bgControl[id] &= ~bits;
}

void bgSetPriority(int id, unsigned int priority)
{
    sassert(priority < 4, "Priority must be less than 4");

    *bgControl[id] &= ~3;
    *bgControl[id] |= priority;
}

void bgSetMapBase(int id, unsigned int base)
{
    sassert(base <= 31, "Map base cannot exceed 31");

    *bgControl[id] &= ~(31 << MAP_BASE_SHIFT);
    *bgControl[id] |= base << MAP_BASE_SHIFT;
}

void bgSetTileBase(int id, unsigned int base)
{
    sassert(base <= 15, "Tile base cannot exceed 15");

    *bgControl[id] &= ~(15 << TILE_BASE_SHIFT);
    *bgControl[id] |= base << TILE_BASE_SHIFT;
}

void bgSetScrollf(int id, s32 x, s32 y)
{
    bgState[id].scrollX = x;
    bgState[id].scrollY = y;

    bgState[id].dirty = true;
}

void bgSetMosaic(unsigned int dx, unsigned int dy)
{
    sassert(dx < 16 && dy < 16, "Mosaic range is 0 to 15");

    mosaicShadow = (mosaicShadow & 0xff00) | (dx | (dy << 4));
    REG_MOSAIC = mosaicShadow;
}

void bgSetMosaicSub(unsigned int dx, unsigned int dy)
{
    sassert(dx < 16 && dy < 16, "Mosaic range is 0 to 15");

    mosaicShadowSub = (mosaicShadowSub & 0xff00) | (dx | (dy << 4));
    REG_MOSAIC_SUB = mosaicShadowSub;
}

u16 *bgGetMapPtr(int id)
{
    if (id < 4)
    {
        int step = DISPLAY_SCREEN_BASE_GET(REG_DISPCNT);
        return (u16 *)BG_MAP_RAM_MAIN(bgGetMapBase(id), step);
    }
    else
    {
        return (u16 *)BG_MAP_RAM_SUB(bgGetMapBase(id));
    }
}

u16 *bgGetGfxPtr(int id)
{
    if (bgState[id].type < BgType_Bmp8) // Tiled modes
    {
        if (id < 4)
        {
            int step = DISPLAY_CHAR_BASE_GET(REG_DISPCNT);
            return (u16 *)BG_TILE_RAM_MAIN(bgGetTileBase(id), step);
        }
        else
        {
            return (u16 *)BG_TILE_RAM_SUB(bgGetTileBase(id));
        }
    }
    else // Bitmap modes
    {
        return (id < 4) ? (u16 *)(BG_GFX + 0x2000 * (bgGetMapBase(id))) :
                          (u16 *)(BG_GFX_SUB + 0x2000 * (bgGetMapBase(id)));
    }
}

void bgSetCenterf(int id, s32 x, s32 y)
{
    sassert(!bgIsText(id), "Text Backgrounds have no Center of Rotation");

    bgState[id].centerX = x;
    bgState[id].centerY = y;

    bgState[id].dirty = true;
}

void bgSetAffineMatrixScroll(int id, int hdx, int vdx, int hdy, int vdy,
                             int scrollx, int scrolly)
{
    sassert(!bgIsText(id), "Text Backgrounds have no affine matrix and scroll registers.");

    bgTransform[id]->hdx = hdx;
    bgTransform[id]->vdx = vdx;
    bgTransform[id]->hdy = hdy;
    bgTransform[id]->vdy = vdy;

    bgTransform[id]->dx = scrollx;
    bgTransform[id]->dy = scrolly;

    bgState[id].dirty = false;
}
