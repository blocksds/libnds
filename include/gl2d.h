// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2010 Richard Eric M. Lope BSN RN (Relminator)

// Easy GL2D
//
// http://rel.betterwebber.com
//
// A very small and simple DS rendering lib using the 3d core to render 2D stuff

/// @file gl2d.h
///
/// @brief A very small and simple DS rendering lib using the 3d core to render
/// 2D stuff.
///
/// @section gl2d API
/// - @ref gl2d.h "Reference"

#ifndef LIBNDS_GL2D_H__
#define LIBNDS_GL2D_H__

#include <nds/arm9/videoGL.h>

/// @brief Enums selecting flipping mode.
///
/// These enums are bits for flipping the sprites.
///
/// You can **"|"** (or) GL_FLIP_V and GL_FLIP_H to flip both ways.
///
/// <ul>
///   <li> Related functions:
///   <ol>
///     <li>glSprite()
///     <li>glSpriteScale()
///     <li>glSpriteRotate()
///     <li>glSpriteScaleXY(()
///     <li>glSpriteRotateScale()
///     <li>glSpriteRotateScaleXY()
///     <li>glSpriteOnQuad()
///   </ol>
/// </ul>

typedef enum
{
    GL_FLIP_NONE = (1 << 0), ///< No flipping
    GL_FLIP_V    = (1 << 1), ///< Sprite is rendered vertically flipped
    GL_FLIP_H    = (1 << 2), ///< Sprite is rendered horizontally flipped
} GL_FLIP_MODE;


/// Struct for our GL-Based Images
///
/// This is our struct to hold our image attributes. You don't need to worry
/// about this if you use Texture Packer.
///
/// <ul>
///   <li> Related functions:
///   <ol>
///     <li>glSprite()
///     <li>glSpriteScale()
///     <li>glSpriteRotate()
///     <li>glSpriteScaleXY(()
///     <li>glSpriteRotateScale()
///     <li>glSpriteRotateScaleXY()
///     <li>glSpriteOnQuad()
///   </ol>
/// </ul>
typedef struct
{
    int width;     ///< Width of the Sprite
    int height;    ///< Height of the Sprite
    int u_off;     ///< S texture offset
    int v_off;     ///< T texture offset

    /// Texture handle (used in glDeleteTextures())
    ///
    /// The texture handle in VRAM (returned by glGenTextures()). ie. This
    /// references the actual texture stored in VRAM.
    int textureID;

} glImage;

#ifdef __cplusplus
extern "C"
{
#endif

extern int gCurrentTexture;

/// Initializes GL in 2D mode.
///
/// Also initializes GL in 3d mode so that we could combine 2D and 3D later.
/// Almost a direct copy from the DS example files.
void glScreen2D(void);

/// Sets up OpenGL for 2d rendering.
///
/// Call this before drawing any of GL2D's drawing or sprite functions.
void glBegin2D(void);

/// Issue this after drawing 2d so that we don't mess the matrix stack.
///
/// The compliment of glBegin2D().
void glEnd2D(void);

/// Returns the active texture. Use with care.
///
/// Needed to achieve some effects since libnds 1.5.0.
///
/// @return Returns the texture ID.
static inline int glGetActiveTexture(void)
{
    return gCurrentTexture;
}

/// Set the active texture. Use with care.
///
/// Needed to achieve some effects since libnds 1.5.0.
///
/// @param TextureID Texture to set as active.
static inline void glSetActiveTexture(int TextureID)
{
    glBindTexture(0, TextureID);
    gCurrentTexture = TextureID;
}

/// Draws a pixel.
///
/// @param x X position of the pixel.
/// @param y Y position of the pixel.
/// @param color RGB15/ARGB16 color.
void glPutPixel(int x, int y, int color);

/// Draws a line.
///
/// @param x1,y1 Top-Left coordinate of the line.
/// @param x2,y2 Bottom-Right coordinate of the line.
/// @param color RGB15/ARGB16 color.
void glLine(int x1, int y1, int x2, int y2, int color);

/// Draws a box.
///
/// @param x1,y1 Top-Left coordinate of the box.
/// @param x2,y2 Bottom-Right coordinate of the box.
/// @param color RGB15/ARGB16 color.
void glBox(int x1, int y1, int x2, int y2, int color);

/// Draws a filled box.
///
/// @param x1,y1 Top-Left coordinate of the box.
/// @param x2,y2 Bottom-Right coordinate of the box.
/// @param color RGB15/ARGB16 color.
void glBoxFilled(int x1, int y1, int x2, int y2, int color);

/// Draws a filled box in gradient colors.
///
/// @param x1,y1 Top-Left coordinate of the box.
/// @param x2,y2 Bottom-Right coordinate of the box.
/// @param color1 RGB15/ARGB16 color of the Top-Left corner.
/// @param color2 RGB15/ARGB16 color of the Bottom-Left corner.
/// @param color3 RGB15/ARGB16 color of the Bottom-Right corner.
/// @param color4 RGB15/ARGB16 color of the Top-Right corner.
void glBoxFilledGradient(int x1, int y1, int x2, int y2,
                         int color1, int color2, int color3, int color4);

/// Draws a triangle.
///
/// @param x1,y1 Vertex 1 of the triangle.
/// @param x2,y2 Vertex 2 of the triangle.
/// @param x3,y3 Vertex 3 of the triangle.
/// @param color RGB15/ARGB16 color of the triangle.
void glTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color);

/// Draws a filled triangle.
///
/// @param x1,y1 Vertex 1 of the triangle.
/// @param x2,y2 Vertex 2 of the triangle.
/// @param x3,y3 Vertex 3 of the triangle.
/// @param color RGB15/ARGB16 color of the triangle.
void glTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, int color);

/// Draws a triangle in gradient colors.
///
/// @param x1,y1 Vertex 1 of the triangle.
/// @param x2,y2 Vertex 2 of the triangle.
/// @param x3,y3 Vertex 3 of the triangle.
/// @param color1 RGB15/ARGB16 color of the vertex 1.
/// @param color2 RGB15/ARGB16 color of the vertex 2.
/// @param color3 RGB15/ARGB16 color of the vertex 3.
void glTriangleFilledGradient(int x1, int y1, int x2, int y2, int x3, int y3,
                              int color1, int color2, int color3);

/// Draws a sprite.
///
/// @param x X position of the sprite.
/// @param y Y position of the sprite.
/// @param flipmode Mode for flipping (see GL_FLIP_MODE enum).
/// @param spr Pointer to a glImage.
void glSprite(int x, int y, int flipmode, const glImage *spr);

/// Draws a scaled sprite.
///
/// @param x X position of the sprite.
/// @param y Y position of the sprite.
/// @param scale 20.12 fixed-point scale value (1 << 12 is normal).
/// @param flipmode Mode for flipping (see GL_FLIP_MODE enum).
/// @param spr Pointer to a glImage.
void glSpriteScale(int x, int y, s32 scale, int flipmode, const glImage *spr);

/// Draws an axis exclusive scaled sprite.
///
/// @param x X position of the sprite.
/// @param y Y position of the sprite.
/// @param scaleX 20.12 fixed-point X-Axis scale value (1 << 12 is normal).
/// @param scaleY 20.12 fixed-point Y-Axis scale value (1 << 12 is normal).
/// @param flipmode Mode for flipping (see GL_FLIP_MODE enum).
/// @param spr Pointer to a glImage.
void glSpriteScaleXY(int x, int y, s32 scaleX, s32 scaleY, int flipmode, const glImage *spr);

/// Draws a center rotated sprite.
///
/// @param x X position of the sprite center.
/// @param y Y position of the sprite center.
/// @param angle Binary Radian Angle(-32768 to 32767) to rotate the sprite.
/// @param flipmode Mode for flipping (see GL_FLIP_MODE enum).
/// @param spr Pointer to a glImage.
void glSpriteRotate(int x, int y, s32 angle, int flipmode, const glImage *spr);

/// Draws a center rotated scaled sprite.
///
/// @param x X position of the sprite center.
/// @param y Y position of the sprite center.
/// @param angle Binary Radian Angle(-32768 to 32767) to rotate the sprite.
/// @param scale 20.12 fixed-point scale value (1 << 12 is normal).
/// @param flipmode Mode for flipping (see GL_FLIP_MODE enum).
/// @param spr Pointer to a glImage.
void glSpriteRotateScale(int x, int y, s32 angle, s32 scale, int flipmode,
                         const glImage *spr);

/// Draws a center rotated axis-exclusive scaled sprite.
///
/// @param x X position of the sprite center.
/// @param y Y position of the sprite center.
/// @param angle Binary Radian Angle(-32768 to 32767) to rotate the sprite.
/// @param scaleX 20.12 fixed-point X-Axis scale value (1 << 12 is normal).
/// @param scaleY 20.12 fixed-point Y-Axis scale value (1 << 12 is normal).
/// @param flipmode Mode for flipping (see GL_FLIP_MODE enum).
/// @param spr Pointer to a glImage.
void glSpriteRotateScaleXY(int x, int y, s32 angle, s32 scaleX, s32 scaleY,
                           int flipmode, const glImage *spr);

/// Draws a horizontaly stretched sprite (clean stretching).
///
/// Useful for "laser effects".
///
/// @param x X position of the sprite center.
/// @param y Y position of the sprite center.
/// @param length_x The length(in pixels) to stretch the sprite.
/// @param spr Pointer to a glImage.
void glSpriteStretchHorizontal(int x, int y, int length_x, const glImage *spr);

/// Draws a horizontaly stretched sprite (clean stretching).
///
/// Useful for "shrearing effects".
///
/// @param x1,y1 First corner of the sprite.
/// @param x2,y2 Second corner of the sprite.
/// @param x3,y3 Third corner of the sprite.
/// @param x4,y4 Fourth corner of the sprite.
/// @param uoff,voff Texture offsets.
/// @param flipmode Mode for flipping (see GL_FLIP_MODE enum).
/// @param spr Pointer to a glImage.
void glSpriteOnQuad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
                    int uoff, int voff, int flipmode, const glImage *spr);

/// Initializes our spriteset with Texture Packer generated UV coordinates.
///
/// Very safe and easy to use.
///
/// @param sprite Pointer to an array of glImage.
/// @param numframes number of frames in a spriteset (auto-generated by Texture
///                  Packer).
/// @param texcoords Texture Packer auto-generated array of UV coords.
/// @param type The format of the texture (see glTexImage2d()).
/// @param sizeX The horizontal size of the texture; valid sizes are enumerated
///              in GL_TEXTURE_TYPE_ENUM (see glTexImage2d()).
/// @param sizeY The vertical size of the texture; valid sizes are enumerated in
///              GL_TEXTURE_TYPE_ENUM (see glTexImage2d()).
/// @param param Parameters for the texture (see glTexImage2d()).
/// @param pallette_width Length of the palette. Valid values are 4, 16, 32, 256
///                       (if 0, the palette is removed from the currently bound
///                       texture).
/// @param palette Pointer to the palette data to load (if NULL, then palette is
///                emoved from currently bound texture).
/// @param texture Pointer to the texture data to load.
/// @return Returns the texture ID.
int glLoadSpriteSet(glImage *sprite, const unsigned int numframes,
                     const unsigned int *texcoords, GL_TEXTURE_TYPE_ENUM type,
                     int sizeX, int sizeY, int param, int pallette_width,
                     const u16 *palette, const uint8_t *texture);

/// Initializes our tileset (like glInitSpriteset()) but without the use of
/// Texture Packer auto-generated files.
///
/// Can only be used when tiles in a tilset are of the same dimensions.
///
/// @param sprite Pointer to an array of glImage.
/// @param tile_wid Width of each tile in the texture.
/// @param tile_hei Height of each tile in the texture.
/// @param bmp_wid Width of of the texture or tileset.
/// @param bmp_hei height of of the texture or tileset.
/// @param type The format of the texture (see glTexImage2d()).
/// @param sizeX The horizontal size of the texture; valid sizes are enumerated
///              in GL_TEXTURE_TYPE_ENUM (see glTexImage2d()).
/// @param sizeY The vertical size of the texture; valid sizes are enumerated in
///              GL_TEXTURE_TYPE_ENUM (see glTexImage2d()).
/// @param param Parameters for the texture (see glTexImage2d()).
/// @param pallette_width Length of the palette. Valid values are 4, 16, 32, 256
///                       (if 0, the palette is removed from the currently bound
///                       texture).
/// @param palette Pointer to the palette data to load (if NULL, then palette
///                 is removed from currently bound texture).
/// @param texture Pointer to the texture data to load.
/// @return Returns the texture ID.
int glLoadTileSet(glImage *sprite, int tile_wid, int tile_hei, int bmp_wid, int bmp_hei,
                  GL_TEXTURE_TYPE_ENUM type, int sizeX, int sizeY, int param,
                  int pallette_width, const u16 *palette, const uint8_t *texture);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_GL2D_H__
