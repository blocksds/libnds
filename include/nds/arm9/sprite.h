// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2007 Liran Nuna (LiraNuna)
// Copyright (C) 2007 Dave Murphy (WinterMute)

/// @file nds/arm9/sprite.h
///
/// @brief NDS sprite helpers.

#ifndef LIBNDS_NDS_ARM9_SPRITE_H__
#define LIBNDS_NDS_ARM9_SPRITE_H__

#ifndef ARM9
#error Sprites are only available on the ARM9
#endif

#include <nds/arm9/video.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

// Sprite control defines

// Attribute 0 consists of 8 bits of Y plus the following flags:
#define ATTR0_NORMAL            (0 << 8)
#define ATTR0_ROTSCALE          (1 << 8)
#define ATTR0_DISABLED          (2 << 8)
#define ATTR0_ROTSCALE_DOUBLE   (3 << 8)

#define ATTR0_TYPE_NORMAL       (0 << 10)
#define ATTR0_TYPE_BLENDED      (1 << 10)
#define ATTR0_TYPE_WINDOWED     (2 << 10)
#define ATTR0_BMP               (3 << 10)

#define ATTR0_MOSAIC            (1 << 12)

#define ATTR0_COLOR_16          (0 << 13) // 16 color in tile mode, 16 bit in bitmap mode
#define ATTR0_COLOR_256         (1 << 13)

#define ATTR0_SQUARE            (0 << 14)
#define ATTR0_WIDE              (1 << 14)
#define ATTR0_TALL              (2 << 14)

#define OBJ_Y(m)                ((m) & 0x00ff)

// Atribute 1 consists of 9 bits of X plus the following flags:
#define ATTR1_ROTDATA(n)        ((n) << 9) // NOTE: overlaps with flip flags
#define ATTR1_FLIP_X            (1 << 12)
#define ATTR1_FLIP_Y            (1 << 13)
#define ATTR1_SIZE_8            (0 << 14)
#define ATTR1_SIZE_16           (1 << 14)
#define ATTR1_SIZE_32           (2 << 14)
#define ATTR1_SIZE_64           (3 << 14)

#define OBJ_X(m)                ((m) & 0x01ff)

// Atribute 2 consists of the following:
#define ATTR2_PRIORITY(n)       ((n) << 10)
#define ATTR2_PALETTE(n)        ((n) << 12)
#define ATTR2_ALPHA(n)          ((n) << 12)

/// The blending mode of the sprite
typedef enum
{
    OBJMODE_NORMAL,     ///< No special mode is on, normal sprite state.
    OBJMODE_BLENDED,    ///< Color blending is on, sprite can use HW blending features.
    OBJMODE_WINDOWED,   ///< Sprite can be seen only inside the sprite window.
    OBJMODE_BITMAP,     ///< Sprite is not using tiles, per pixel image data.

} ObjBlendMode;

/// The shape of the sprite
typedef enum {
    OBJSHAPE_SQUARE,    ///< Sprite shape is NxN (Height == Width).
    OBJSHAPE_WIDE,      ///< Sprite shape is NxM with N > M (Height < Width).
    OBJSHAPE_TALL,      ///< Sprite shape is NxM with N < M (Height > Width).
    OBJSHAPE_FORBIDDEN, ///< Sprite shape is undefined.
} ObjShape;

/// The size of the sprite
typedef enum {
    OBJSIZE_8,          ///< Major sprite size is 8px.
    OBJSIZE_16,         ///< Major sprite size is 16px.
    OBJSIZE_32,         ///< Major sprite size is 32px.
    OBJSIZE_64,         ///< Major sprite size is 64px.
} ObjSize;

/// The color mode of the sprite
typedef enum {
    OBJCOLOR_16,        ///< Sprite has 16 colors.
    OBJCOLOR_256,       ///< Sprite has 256 colors.
} ObjColMode;

/// The priority of the sprite
typedef enum {
    OBJPRIORITY_0,      ///< Sprite priority level 0 - highest.
    OBJPRIORITY_1,      ///< Sprite priority level 1.
    OBJPRIORITY_2,      ///< Sprite priority level 2.
    OBJPRIORITY_3,      ///< Sprite priority level 3 - lowest.
} ObjPriority;

/// A bitfield of sprite attributes
typedef union SpriteEntry
{
    struct {
        struct {
            u16 y                           : 8; ///< Sprite Y position.
            union {
                struct {
                    u8                      : 1;
                    bool isHidden           : 1; ///< Sprite is hidden (isRotoscale cleared).
                    u8                      : 6;
                };
                struct {
                    bool isRotateScale      : 1; ///< Sprite uses affine parameters if set.
                    bool isSizeDouble       : 1; ///< Sprite bounds is doubled (isRotoscale set).
                    ObjBlendMode blendMode  : 2; ///< Sprite object mode.
                    bool isMosaic           : 1; ///< Enables mosaic effect if set.
                    ObjColMode colorMode    : 1; ///< Sprite color mode.
                    ObjShape shape          : 2; ///< Sprite shape.
                };
            };
        };

        union {
            struct {
                u16 x                    : 9; ///< Sprite X position.
                u8                       : 7;
            };
            struct {
                u8                       : 8;
                union {
                    struct {
                        u8               : 4;
                        bool hFlip       : 1; ///< Flip sprite horizontally (isRotoscale cleared).
                        bool vFlip       : 1; ///< Flip sprite vertically (isRotoscale cleared).
                        u8               : 2;
                    };
                    struct {
                        u8               : 1;
                        u8 rotationIndex : 5; ///< Affine parameter number to use (isRotoscale set).
                        ObjSize size     : 2; ///< Sprite size.
                    };
                };
            };
        };

        struct {
            u16 gfxIndex         : 10; ///< Upper-left tile index.
            ObjPriority priority : 2;  ///< Sprite priority.
            u8 palette           : 4;  ///< Sprite palette to use in paletted color modes.
        };

        u16 attribute3; // Unused. Four of those are used as a sprite rotation matrix
    };

    struct {
        uint16_t attribute[3];
        uint16_t filler;
    };

} SpriteEntry, *pSpriteEntry;

/// A sprite rotation entry.
typedef struct SpriteRotation
{
    uint16_t filler1[3]; // Filler for the sprite entry attributes which overlap these.
    int16_t hdx;         ///< The change in x per horizontal pixel.

    uint16_t filler2[3]; // Filler for the sprite entry attributes which overlap these.
    int16_t vdx;         ///< The change in y per horizontal pixel.

    uint16_t filler3[3]; // Filler for the sprite entry attributes which overlap these.
    int16_t hdy;         ///< The change in x per vertical pixel.

    uint16_t filler4[3]; // Filler for the sprite entry attributes which overlap these.
    int16_t vdy;         ///< The change in y per vertical pixel.
} SpriteRotation, *pSpriteRotation;

/// Maximum number of sprites per engine available.
#define SPRITE_COUNT    128

/// Maximum number of affine matrices per engine available.
#define MATRIX_COUNT    32

// TODO: Is this union still used?
typedef union OAMTable
{
    SpriteEntry oamBuffer[SPRITE_COUNT];
    SpriteRotation matrixBuffer[MATRIX_COUNT];
} OAMTable;

/// Enumerates all sizes supported by the 2D engine.
typedef enum
{
    SpriteSize_8x8   = (OBJSIZE_8 << 14) | (OBJSHAPE_SQUARE << 12) | (8 * 8 >> 5), //!< 8x8
    SpriteSize_16x16 = (OBJSIZE_16 << 14) | (OBJSHAPE_SQUARE << 12) | (16 * 16 >> 5), //!< 16x16
    SpriteSize_32x32 = (OBJSIZE_32 << 14) | (OBJSHAPE_SQUARE << 12) | (32 * 32 >> 5), //!< 32x32
    SpriteSize_64x64 = (OBJSIZE_64 << 14) | (OBJSHAPE_SQUARE << 12) | (64 * 64 >> 5), //!< 64x64

    SpriteSize_16x8  = (OBJSIZE_8 << 14)  | (OBJSHAPE_WIDE << 12) | (16 * 8 >> 5), //!< 16x8
    SpriteSize_32x8  = (OBJSIZE_16 << 14) | (OBJSHAPE_WIDE << 12) | (32 * 8 >> 5), //!< 32x8
    SpriteSize_32x16 = (OBJSIZE_32 << 14) | (OBJSHAPE_WIDE << 12) | (32 * 16 >> 5), //!< 32x16
    SpriteSize_64x32 = (OBJSIZE_64 << 14) | (OBJSHAPE_WIDE << 12) | (64 * 32 >> 5), //!< 64x32

    SpriteSize_8x16  = (OBJSIZE_8 << 14)  | (OBJSHAPE_TALL << 12) | (8 * 16 >> 5), //!< 8x16
    SpriteSize_8x32  = (OBJSIZE_16 << 14) | (OBJSHAPE_TALL << 12) | (8 * 32 >> 5), //!< 8x32
    SpriteSize_16x32 = (OBJSIZE_32 << 14) | (OBJSHAPE_TALL << 12) | (16 * 32 >> 5), //!< 16x32
    SpriteSize_32x64 = (OBJSIZE_64 << 14) | (OBJSHAPE_TALL << 12) | (32 * 64 >> 5) //!< 32x64

} SpriteSize;

#define SPRITE_SIZE_SHAPE(size)     (((size) >> 12) & 0x3)
#define SPRITE_SIZE_SIZE(size)      (((size) >> 14) & 0x3)
#define SPRITE_SIZE_PIXELS(size)    (((size) & 0xFFF) << 5)

/// Graphics memory layout options.
typedef enum
{
    /// 1D tile mapping 32 byte boundary between offset
    SpriteMapping_1D_32 = DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_32 | (0 << 28) | 0,

    /// 1D tile mapping 64 byte boundary between offset
    SpriteMapping_1D_64 = DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_64 | (1 << 28) | 1,

    /// 1D tile mapping 128 byte boundary between offset
    SpriteMapping_1D_128 = DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_128 | (2 << 28) | 2,

    /// 1D tile mapping 256 byte boundary between offset
    SpriteMapping_1D_256 = DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_256 | (3 << 28) | 3,

    /// 2D tile mapping 32 byte boundary between offset
    SpriteMapping_2D = DISPLAY_SPR_2D | (4 << 28),

    /// 1D bitmap mapping 128 byte boundary between offset
    SpriteMapping_Bmp_1D_128 = DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_128
        | DISPLAY_SPR_1D_BMP |DISPLAY_SPR_1D_BMP_SIZE_128 | (5 << 28) | 2,

    /// 1D bitmap mapping 256 byte boundary between offset
    SpriteMapping_Bmp_1D_256 = DISPLAY_SPR_1D | DISPLAY_SPR_1D_SIZE_256
        | DISPLAY_SPR_1D_BMP |DISPLAY_SPR_1D_BMP_SIZE_256 | (6 << 28) | 3,

    /// 2D bitmap mapping 128 pixels wide bitmap
    SpriteMapping_Bmp_2D_128 = DISPLAY_SPR_2D | DISPLAY_SPR_2D_BMP_128 | (7 << 28) | 2,

    /// 2D bitmap mapping 256 pixels wide bitmap
    SpriteMapping_Bmp_2D_256 = DISPLAY_SPR_2D | DISPLAY_SPR_2D_BMP_256 | (int)(8U << 28) | 3
} SpriteMapping;

/// Color formats for sprite graphics.
typedef enum {
   SpriteColorFormat_16Color = OBJCOLOR_16,     ///< 16 colors per sprite
   SpriteColorFormat_256Color = OBJCOLOR_256,   ///< 256 colors per sprite
   SpriteColorFormat_Bmp = OBJMODE_BITMAP       ///< 16-bit sprites
} SpriteColorFormat;

typedef struct AllocHeader
{
   u16 nextFree;
   u16 size;
} AllocHeader;

/// Holds the state for a 2D sprite engine.
///
/// There are two of these objects, oamMain and oamSub and these must be passed
/// in to all oam functions.
typedef struct OamState
{
    int gfxOffsetStep;        ///< The distance between tiles as 2^gfxOffsetStep
    s16 firstFree;            ///< Pointer to the first free block of tiles
    s16 allocBufferSize;      ///< Current size of the allocation buffer
    AllocHeader *allocBuffer; ///< Array, allocation buffer for graphics allocation
    union
    {
        SpriteEntry *oamMemory;            ///< Pointer to shadow oam memory
        SpriteRotation *oamRotationMemory; ///< Pointer to shadow oam memory for rotation
    };
    SpriteMapping spriteMapping; ///< The mapping of the OAM.
} OamState;

#ifdef __cplusplus
extern "C" {
#endif

/// An object representing the main 2D engine.
extern OamState oamMain;

/// An object representing the sub 2D engine.
extern OamState oamSub;

/// Convert a VRAM address to an OAM offset
///
/// @param oam Must be &oamMain or &oamSub.
/// @param offset The VRAM of the sprite graphics (not an offset).
/// @return OAM offset.
unsigned int oamGfxPtrToOffset(OamState *oam, const void *offset);

/// Initializes the 2D sprite engine.
///
/// In order to mix tiled and bitmap sprites use SpriteMapping_Bmp_1D_128 or
/// SpriteMapping_Bmp_1D_256. This will set mapping for both to 1D and give same
/// sized boundaries so the sprite gfx allocation will function. VBlank IRQ must
/// be enabled for this function to work.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param mapping The mapping mode.
/// @param extPalette If true it sets up extended palettes for 8 bpp sprites.
void oamInit(OamState *oam, SpriteMapping mapping, bool extPalette);

/// Disables sprite rendering.
///
/// @param oam Must be &oamMain or &oamSub.
void oamDisable(OamState *oam);

/// Enables sprite rendering.
///
/// @param oam Must be &oamMain or &oamSub.
void oamEnable(OamState *oam);

/// Translates an OAM offset into a VRAM address.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param gfxOffsetIndex The index to compute.
/// @return The address in VRAM corresponding to the supplied offset.
u16 *oamGetGfxPtr(OamState *oam, int gfxOffsetIndex);

/// Allocates graphics memory for the supplied sprite attributes.
///
/// @param oam Must be &oamMain or &oamSub..
/// @param size The size of the sprite to allocate.
/// @param colorFormat The color format of the sprite.
/// @return The address in VRAM of the allocated sprite.
u16* oamAllocateGfx(OamState *oam, SpriteSize size, SpriteColorFormat colorFormat);

/// Free VRAM memory obtained with oamAllocateGfx.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param gfxOffset A VRAM offset obtained from oamAllocateGfx.
void oamFreeGfx(OamState *oam, const void* gfxOffset);

/// Sets engine A global sprite mosaic.
///
/// @param dx (0-15) horizontal mosaic value.
/// @param dy (0-15) horizontal mosaic value.
static inline void oamSetMosaic(unsigned int dx, unsigned int dy)
{
    sassert(dx < 16 && dy < 16, "Mosaic range must be 0 to 15");

    mosaicShadow = (mosaicShadow & 0x00ff) | (dx << 8)| (dy << 12);
    REG_MOSAIC = mosaicShadow;
}

/// Sets engine B global sprite mosaic.
///
/// @param dx (0-15) horizontal mosaic value.
/// @param dy (0-15) horizontal mosaic value.
static inline void oamSetMosaicSub(unsigned int dx, unsigned int dy)
{
    sassert(dx < 16 && dy < 16, "Mosaic range must be 0 to 15");

    mosaicShadowSub = (mosaicShadowSub & 0x00ff) | (dx << 8)| (dy << 12);
    REG_MOSAIC_SUB = mosaicShadowSub;
}

/// Sets an OAM entry to the supplied values.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param x The x location of the sprite in pixels.
/// @param y The y location of the sprite in pixels.
/// @param priority The sprite priority (0 to 3).
/// @param palette_alpha The palette number for 4bpp and 8bpp (extended palette
///                      mode), or the alpha value for bitmap sprites (bitmap
///                      sprites must specify a value > 0 to display) [0 - 15].
/// @param size The size of the sprite
/// @param format The color format of the sprite
/// @param gfxOffset The VRAM address of the sprite graphics (not an offset).
/// @param affineIndex Affine index to use [0 - 31].
/// @param sizeDouble If affineIndex >= 0 this will be used to double the sprite
///                   size for rotation.
/// @param hide If non zero (true) the sprite will be hidden.
/// @param vflip Flip the sprite vertically.
/// @param hflip Flip the sprite horizontally.
/// @param mosaic If true mosaic will be applied to the sprite.
void oamSet(OamState *oam, int id,  int x, int y, int priority,
            int palette_alpha, SpriteSize size, SpriteColorFormat format,
            const void *gfxOffset, int affineIndex, bool sizeDouble, bool hide,
            bool hflip, bool vflip, bool mosaic);

/// Sets an OAM entry to the supplied (x, y) position.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param x The x location of the sprite in pixels.
/// @param y The y location of the sprite in pixels.
static inline void oamSetXY(OamState* oam, int id, int x, int y)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetXY() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetXY() index is out of bounds, must be 0-127");

    oam->oamMemory[id].x = x;
    oam->oamMemory[id].y = y;
}

/// Sets an OAM entry to the supplied priority.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param priority the sprite priority [0 - 3].
static inline void oamSetPriority(OamState *oam, int id, int priority)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetPriority() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetPriority() index is out of bounds, must be 0-127");
    sassert(priority >= 0 && priority < 4,
            "oamSetPriority() priority is out of bounds, must be 0-3");

    oam->oamMemory[id].priority = (ObjPriority)priority;
}

/// Sets a paletted OAM entry to the supplied palette.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param palette The palette number for 4bpp and 8bpp (extended palette mode)
///                sprites [0 - 15].
static inline void oamSetPalette(OamState *oam, int id, int palette)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetPalette() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetPalette() index is out of bounds, must be 0-127");
    sassert(palette >= 0 && palette < 16,
            "oamSetPalette() palette is out of bounds, must be 0-15");
    sassert(oam->oamMemory[id].blendMode != (ObjBlendMode)SpriteColorFormat_Bmp,
            "oamSetPalette() cannot set palette on a bitmapped sprite");

    oam->oamMemory[id].palette = palette;
}

/// Sets a bitmapped OAM entry to the supplied transparency.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param alpha The alpha value for bitmap sprites (bitmap sprites must specify
///              a value > 0 to display) [0 - 15].
static inline void oamSetAlpha(OamState *oam, int id, int alpha)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetAlpha() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetAlpha() index is out of bounds, must be 0-127");
    sassert(alpha >= 0 && alpha < 16,
            "oamSetAlpha() alpha is out of bounds, must be 0-15");
    sassert(oam->oamMemory[id].blendMode == (ObjBlendMode)SpriteColorFormat_Bmp,
            "oamSetAlpha() cannot set alpha on a paletted sprite");

    oam->oamMemory[id].palette = alpha;
}

/// Sets an OAM entry to the supplied shape/size/pointer.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param size The size of the sprite.
/// @param format The color format of the sprite.
/// @param gfxOffset The VRAM address of the sprite graphics (not an offset).
static inline void oamSetGfx(OamState *oam, int id, SpriteSize size,
                             SpriteColorFormat format, const void* gfxOffset)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetGfx() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetGfx() index is out of bounds, must be 0-127");

    oam->oamMemory[id].shape    = (ObjShape)SPRITE_SIZE_SHAPE(size);
    oam->oamMemory[id].size     = (ObjSize)SPRITE_SIZE_SIZE(size);
    oam->oamMemory[id].gfxIndex = oamGfxPtrToOffset(oam, gfxOffset);

    if (format != SpriteColorFormat_Bmp)
    {
        oam->oamMemory[id].colorMode = (ObjColMode)format;
    }
    else
    {
        oam->oamMemory[id].blendMode = (ObjBlendMode)format;
        oam->oamMemory[id].colorMode = (ObjColMode)0;
    }
}

/// Sets an OAM entry to the supplied affine index.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param affineIndex Affine index to use [0 - 31].
/// @param sizeDouble If affineIndex >= 0 and < 32 this will be used to double
///                   the sprite size for rotation.
static inline void oamSetAffineIndex(OamState *oam, int id, int affineIndex,
                                     bool sizeDouble)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetAffineIndex() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetAffineIndex() index is out of bounds, must be 0-127");

    if (affineIndex >= 0 && affineIndex < 32)
    {
        oam->oamMemory[id].rotationIndex = affineIndex;
        oam->oamMemory[id].isSizeDouble  = sizeDouble;
        oam->oamMemory[id].isRotateScale = true;
    }
    else
    {
        oam->oamMemory[id].isSizeDouble  = false;
        oam->oamMemory[id].isRotateScale = false;
    }
}

/// Sets an OAM entry to the supplied hidden state.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param hide If non zero (true) the sprite will be hidden.
static inline void oamSetHidden(OamState *oam, int id, bool hide)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetHidden() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetHidden() index is out of bounds, must be 0-127");
    sassert(!oam->oamMemory[id].isRotateScale,
            "oamSetHidden() cannot set hide on a RotateScale sprite");

    oam->oamMemory[id].isHidden = hide ? true : false;
}

/// Sets an OAM entry to the supplied flipping.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param hflip Flip the sprite horizontally.
/// @param vflip Flip the sprite vertically.
static inline void oamSetFlip(OamState *oam, int id, bool hflip, bool vflip)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetFlip() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetFlip() index is out of bounds, must be 0-127");
    sassert(!oam->oamMemory[id].isRotateScale,
            "oamSetFlip() cannot set flip on a RotateScale sprite");

    oam->oamMemory[id].hFlip = hflip ? true : false;
    oam->oamMemory[id].vFlip = vflip ? true : false;
}

/// Sets an OAM entry to enable or disable mosaic.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param id The OAM number to be set [0 - 127].
/// @param mosaic If true mosaic will be applied to the sprite.
static inline void oamSetMosaicEnabled(OamState *oam, int id, bool mosaic)
{
    sassert(oam == &oamMain || oam == &oamSub,
            "oamSetMosaicEnabled() oam must be &oamMain or &oamSub");
    sassert(id >= 0 && id < SPRITE_COUNT,
            "oamSetMosaicEnabled() index is out of bounds, must be 0-127");

    oam->oamMemory[id].isMosaic = mosaic ? true : false;
}

/// Hides the sprites in the supplied range.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param start The first index to clear.
/// @param count The number of sprites to clear (zero will clear all sprites).
void oamClear(OamState *oam, int start, int count);

/// Hides a single sprite.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param index The index of the sprite [0 - 127].
static inline void oamClearSprite(OamState *oam, int index)
{
    sassert(index >= 0 && index < SPRITE_COUNT,
            "oamClearSprite() index is out of bounds, must be 0-127");

    oam->oamMemory[index].attribute[0] = ATTR0_DISABLED;
}

/// Causes OAM to be updated.
///
/// It must be called during vblank if using the OAM API.
///
/// @param oam Must be &oamMain or &oamSub.
void oamUpdate(OamState *oam);

/// Sets the specified rotation scale entry.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param rotId The rotation entry to set.
/// @param angle The ccw angle to rotate [-32768 - 32767].
/// @param sx The inverse scale factor in the x direction.
/// @param sy The inverse scale factor in the y direction.
void oamRotateScale(OamState *oam, int rotId, int angle, int sx, int sy);

/// Allows you to directly set the affine transformation matrix.
///
/// With this, you have more freedom to set the matrix, but it might be more
/// difficult to use if you're not used to affine transformation matrix. This
/// will erase the previous matrix stored at rotId.
///
/// @param oam Must be &oamMain or &oamSub.
/// @param rotId The ID of the rotscale item you want to change [0 - 31].
/// @param hdx The change in x per horizontal pixel.
/// @param hdy The change in y per horizontal pixel.
/// @param vdx The change in x per vertical pixel.
/// @param vdy The change in y per vertical pixel.
static inline void oamAffineTransformation(OamState *oam, int rotId, int hdx, int hdy,
                                           int vdx, int vdy)
{
    sassert(rotId >= 0 && rotId < 32,
            "oamAffineTransformation() rotId is out of bounds, must be 0-31");

    oam->oamRotationMemory[rotId].hdx = hdx;
    oam->oamRotationMemory[rotId].vdx = vdx;
    oam->oamRotationMemory[rotId].hdy = hdy;
    oam->oamRotationMemory[rotId].vdy = vdy;
}

/// Determines the number of fragments in the allocation engine.
///
/// @param oam Must be &oamMain or &oamSub.
/// @return The number of fragments.
int oamCountFragments(OamState *oam);

void oamAllocReset(OamState *oam);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_SPRITE_H__
