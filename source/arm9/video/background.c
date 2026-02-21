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

// Initializes and enables the appropriate background with the supplied
// attributes returns an id which must be supplied to the remainder of the
// background functions.
int bgInit(int layer, BgType type, BgSize size, int mapBase, int tileBase)
{
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

    videoBgEnable(layer);

    bgState[layer].dirty = true;

    bgUpdate();

    return layer;
}

int bgInitSub(int layer, BgType type, BgSize size, int mapBase, int tileBase)
{
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

    videoBgEnableSub(layer);

    bgState[layer + 4].dirty = true;

    bgUpdate();

    return layer + 4;
}
