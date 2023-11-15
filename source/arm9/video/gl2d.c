// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2010 Richard Eric M. Lope BSN RN (Relminator)

// Easy GL2D
//
// http://rel.betterwebber.com
//
// A very small and simple DS rendering lib using the 3d core to render 2D stuff

#include <gl2d.h>

// Direct copy of glVertex3v16(). I made this since VideoGL don't have a
// glVertex#i() wrappers.
static inline void gxVertex3i(v16 x, v16 y, v16 z)
{
    GFX_VERTEX16 = (y << 16) | (x & 0xFFFF);
    GFX_VERTEX16 = ((uint32_t)(uint16_t)z);
}

// Again no gxVertex2i() in the videoGL header. This is used for optimizing
// vertex calls.
static inline void gxVertex2i(v16 x, v16 y)
{
    GFX_VERTEX_XY = (y << 16) | (x & 0xFFFF);
}

// Almost a direct copy of TEXTURE_PACK except that UV coords are shifted left
// by 4 bits.  U and V are shifted left by 4 bits since GFX_TEX_COORD expects
// 12.4 Fixed point values.
static inline void gxTexcoord2i(t16 u, t16 v)
{
    GFX_TEX_COORD = (v << 20) | ((u << 4) & 0xFFFF);
}

// I made this since the scale wrappers are either the vectorized mode or does
// not permit you to scale only the axis you want to scale. Needed for sprite
// scaling.
static inline void gxScalef32(s32 x, s32 y, s32 z)
{
    MATRIX_SCALE = x;
    MATRIX_SCALE = y;
    MATRIX_SCALE = z;
}

// I this made for future naming conflicts.
static inline void gxTranslate3f32(int32_t x, int32_t y, int32_t z)
{
    MATRIX_TRANSLATE = x;
    MATRIX_TRANSLATE = y;
    MATRIX_TRANSLATE = z;
}

// Our static global variable used for depth values since we cannot disable
// depth testing in the DS hardware. This value is incremented for every draw
// call.
static v16 g_depth = 0;
int gCurrentTexture = 0;

// Set orthographic projection at 1:1 correspondence to screen coords.
//
// glOrtho expects f32 values but if we use the standard f32 values, we need to
// rescale either every vert or the modelview matrix by the same amount to make
// it work. That's gonna give us lots of overflows and headaches. So we "scale
// down" and use an all integer value.
static inline void SetOrtho(void)
{
    glMatrixMode(GL_PROJECTION);    // Set matrixmode to projection
    glLoadIdentity();               // Reset
    // Downscale projection matrix
    glOrthof32(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -(1 << 12), 1 << 12);
}

void glScreen2D(void)
{
    // Initialize gl
    glInit();

    // Enable textures
    glEnable(GL_TEXTURE_2D);

    // Enable antialiasing
    glEnable(GL_ANTIALIAS);

    // Setup the rear plane
    glClearColor(0, 0, 0, 31); // BG must be opaque for AA to work
    glClearPolyID(63); // BG must have a unique polygon ID for AA to work

    glClearDepth(GL_MAX_DEPTH);

    // This should work the same as the normal gl call
    glViewport(0, 0, 255, 191);

    // Any floating point gl call is being converted to fixed prior to being
    // implemented
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(70, 256.0 / 192.0, 1, 200);

    gluLookAt(0.0, 0.0, 1.0,  // Camera possition
              0.0, 0.0, 0.0,  // Look at
              0.0, 1.0, 0.0); // Up

    glMaterialf(GL_AMBIENT, RGB15(31, 31, 31));
    glMaterialf(GL_DIFFUSE, RGB15(31, 31, 31));
    glMaterialf(GL_SPECULAR, BIT(15) | RGB15(31, 31, 31));
    glMaterialf(GL_EMISSION, RGB15(31, 31, 31));

    // The DS uses a table for shinyness, this generates one
    glMaterialShinyness();

    // Polygon attributes
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
}

void glBegin2D(void)
{
    // Save 3d perpective projection matrix
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    // Save 3d modelview matrix for safety
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // What?!! No glDisable(GL_DEPTH_TEST)?!!!!!!
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_ANTIALIAS);    // Disable AA
    glDisable(GL_OUTLINE);      // Disable edge-marking

    glColor(0x7FFF);            // White

    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE); // No culling

    SetOrtho();

    // Reset texture matrix just in case we did some funky stuff with it
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    // Reset modelview matrix. No need to scale up by << 12
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gCurrentTexture = 0; // Set current texture to 0
    // Set depth to 0. We need this var since we cannot disable depth testing
    g_depth = 0;
}

void glEnd2D(void)
{
    // Restore 3d matrices and set current matrix to modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix(1);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix(1);
}

void glPutPixel(int x, int y, int color)
{
    glBindTexture(0, 0);
    glColor(color);
    glBegin(GL_TRIANGLES);
        gxVertex3i(x, y, g_depth);
        gxVertex2i(x, y);
        gxVertex2i(x, y);
    glEnd();
    glColor(0x7FFF);
    g_depth++;
    gCurrentTexture = 0;
}

void glLine(int x1, int y1, int x2, int y2, int color)
{
    x2++;
    y2++;

    glBindTexture(0, 0);
    glColor(color);
    glBegin(GL_TRIANGLES);
        gxVertex3i(x1, y1, g_depth);
        gxVertex2i(x2, y2);
        gxVertex2i(x2, y2);
    glEnd();
    glColor(0x7FFF);
    g_depth++;
    gCurrentTexture = 0;

}

void glBox(int x1, int y1, int x2, int y2, int color)
{
    x2++;
    y2++;

    glBindTexture(0, 0);
    glColor(color);
    glBegin(GL_TRIANGLES);

        gxVertex3i(x1, y1, g_depth);
        gxVertex2i(x2, y1);
        gxVertex2i(x2, y1);

        gxVertex2i(x2, y1);
        gxVertex2i(x2, y2);
        gxVertex2i(x2, y2);

        // Bug fix for lower-right corner disappearing pixel
        gxVertex2i(++x2, y2);
        gxVertex2i(x1, y2);
        gxVertex2i(x1, y2);

        gxVertex2i(x1, y2);
        gxVertex2i(x1, y1);
        gxVertex2i(x1, y1);

    glEnd();
    glColor(0x7FFF);
    g_depth++;
    gCurrentTexture = 0;

}

void glBoxFilled(int x1, int y1, int x2, int y2, int color)
{
    x2++;
    y2++;

    glBindTexture(0, 0);
    glColor(color);
    glBegin(GL_QUADS);
        // Use 3i for first vertex so that we increment HW depth
        gxVertex3i(x1, y1, g_depth);
        // No need for 3 vertices as 2i would share last depth call
        gxVertex2i(x1, y2);
        gxVertex2i(x2, y2);
        gxVertex2i(x2, y1);
    glEnd();
    glColor(0x7FFF);
    g_depth++;
    gCurrentTexture = 0;
}

void glBoxFilledGradient(int x1, int y1, int x2, int y2,
                         int color1, int color2, int color3, int color4)
{
    x2++;
    y2++;

    glBindTexture(0,0);
    glBegin(GL_QUADS);
        glColor(color1);
        // Use 3i for first vertex so that we increment HW depth
        gxVertex3i(x1, y1, g_depth);
        glColor(color2);
        // No need for 3 vertices as 2i would share last depth call
        gxVertex2i(x1, y2);
        glColor(color3);
        gxVertex2i(x2, y2);
        glColor(color4);
        gxVertex2i(x2, y1);
    glEnd();
    glColor(0x7FFF);
    g_depth++;
    gCurrentTexture = 0;
}

void glTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color)
{
    glBindTexture(0, 0);
    glColor(color);
    glBegin(GL_TRIANGLES);

        gxVertex3i(x1, y1, g_depth);
        gxVertex2i(x2, y2);
        gxVertex2i(x2, y2);

        gxVertex2i(x2, y2);
        gxVertex2i(x3, y3);
        gxVertex2i(x3, y3);

        gxVertex2i(x3, y3);
        gxVertex2i(x1, y1);
        gxVertex2i(x1, y1);

    glEnd();
    glColor(0x7FFF);
    g_depth++;
    gCurrentTexture = 0;
}

void glTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, int color)
{

    glBindTexture(0, 0);
    glColor(color);
    glBegin(GL_TRIANGLES);
        // Use 3i for first vertex so that we increment HW depth
        gxVertex3i(x1, y1, g_depth);
        // No need for 3 vertices as 2i would share last depth call
        gxVertex2i(x2, y2);
        gxVertex2i(x3, y3);
    glEnd();
    glColor(0x7FFF);
    g_depth++;
    gCurrentTexture = 0;

}

void glTriangleFilledGradient(int x1, int y1, int x2, int y2, int x3, int y3,
                              int color1, int color2, int color3)
{
    glBindTexture(0, 0);
    glBegin(GL_TRIANGLES);
        // Use 3i for first vertex so that we increment HW depth
        glColor(color1);
        gxVertex3i(x1, y1, g_depth);
        glColor(color2);
        // No need for 3 vertices as 2i would share last depth call
        gxVertex2i(x2, y2);
        glColor(color3);
        gxVertex2i(x3, y3);
    glEnd();
    glColor(0x7FFF);
    g_depth++;
    gCurrentTexture = 0;
}

void glSprite(int x, int y, int flipmode, const glImage *spr)
{
    int x1 = x;
    int y1 = y;
    int x2 = x + spr->width;
    int y2 = y + spr->height;

    int u1 = spr->u_off + ((flipmode & GL_FLIP_H) ? spr->width - 1 : 0);
    int u2 = spr->u_off + ((flipmode & GL_FLIP_H) ? 0 : spr->width);
    int v1 = spr->v_off + ((flipmode & GL_FLIP_V) ? spr->height - 1 : 0);
    int v2 = spr->v_off + ((flipmode & GL_FLIP_V) ? 0 : spr->height);

    if (spr->textureID != gCurrentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, spr->textureID);
        gCurrentTexture = spr->textureID;
    }

    glBegin(GL_QUADS);

        gxTexcoord2i(u1, v1); gxVertex3i(x1, y1, g_depth);
        gxTexcoord2i(u1, v2); gxVertex2i(x1, y2);
        gxTexcoord2i(u2, v2); gxVertex2i(x2, y2);
        gxTexcoord2i(u2, v1); gxVertex2i(x2, y1);

    glEnd();

    g_depth++;
}

void glSpriteScale(int x, int y, s32 scale, int flipmode, const glImage *spr)
{
    int x1 = 0;
    int y1 = 0;
    int x2 = spr->width;
    int y2 = spr->height;

    int u1 = spr->u_off + ((flipmode & GL_FLIP_H) ? spr->width - 1 : 0);
    int u2 = spr->u_off + ((flipmode & GL_FLIP_H) ? 0 : spr->width - 1);
    int v1 = spr->v_off + ((flipmode & GL_FLIP_V) ? spr->height - 1 : 0);
    int v2 = spr->v_off + ((flipmode & GL_FLIP_V) ? 0 : spr->height - 1);

    if (spr->textureID != gCurrentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, spr->textureID);
        gCurrentTexture = spr->textureID;
    }

    glPushMatrix();

        gxTranslate3f32(x, y, 0);
        gxScalef32(scale, scale, 1 << 12);

        glBegin(GL_QUADS);

            gxTexcoord2i(u1, v1);
            gxVertex3i(x1, y1, g_depth);
            gxTexcoord2i(u1, v2);
            gxVertex2i(x1, y2);
            gxTexcoord2i(u2, v2);
            gxVertex2i(x2, y2);
            gxTexcoord2i(u2, v1);
            gxVertex2i(x2, y1);

        glEnd();

    glPopMatrix(1);
    g_depth++;
}

void glSpriteScaleXY(int x, int y, s32 scaleX, s32 scaleY, int flipmode,
                     const glImage *spr)
{
    int x1 = 0;
    int y1 = 0;
    int x2 = spr->width;
    int y2 = spr->height;

    int u1 = spr->u_off + ((flipmode & GL_FLIP_H) ? spr->width - 1 : 0);
    int u2 = spr->u_off + ((flipmode & GL_FLIP_H) ? 0 : spr->width - 1);
    int v1 = spr->v_off + ((flipmode & GL_FLIP_V) ? spr->height - 1 : 0);
    int v2 = spr->v_off + ((flipmode & GL_FLIP_V) ? 0 : spr->height - 1);

    if (spr->textureID != gCurrentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, spr->textureID);
        gCurrentTexture = spr->textureID;
    }

    glPushMatrix();

        gxTranslate3f32(x, y, 0);
        gxScalef32(scaleX, scaleY, 1 << 12);

        glBegin(GL_QUADS);

            gxTexcoord2i(u1, v1);
            gxVertex3i(x1, y1, g_depth);
            gxTexcoord2i(u1, v2);
            gxVertex2i(x1, y2);
            gxTexcoord2i(u2, v2);
            gxVertex2i(x2, y2);
            gxTexcoord2i(u2, v1);
            gxVertex2i(x2, y1);

        glEnd();

    glPopMatrix(1);
    g_depth++;
}

void glSpriteRotate(int x, int y, s32 angle, int flipmode, const glImage *spr)
{
    int s_half_x = ((spr->width) + (spr->width & 1)) / 2;
    int s_half_y = ((spr->height) + (spr->height & 1)) / 2;

    int x1 = -s_half_x;
    int y1 = -s_half_y;

    int x2 = s_half_x;
    int y2 = s_half_y;

    int u1 = spr->u_off + ((flipmode & GL_FLIP_H) ? spr->width - 1 : 0);
    int u2 = spr->u_off + ((flipmode & GL_FLIP_H) ? 0 : spr->width - 1);
    int v1 = spr->v_off + ((flipmode & GL_FLIP_V) ? spr->height - 1 : 0);
    int v2 = spr->v_off + ((flipmode & GL_FLIP_V) ? 0 : spr->height - 1);

    if (spr->textureID != gCurrentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, spr->textureID);
        gCurrentTexture = spr->textureID;
    }

    glPushMatrix();

        gxTranslate3f32(x, y, 0);
        glRotateZi(angle);

        glBegin(GL_QUADS);

            gxTexcoord2i(u1, v1);
            gxVertex3i(x1, y1, g_depth);
            gxTexcoord2i(u1, v2);
            gxVertex2i(x1, y2);
            gxTexcoord2i(u2, v2);
            gxVertex2i(x2, y2);
            gxTexcoord2i(u2, v1);
            gxVertex2i(x2, y1);

        glEnd();

    glPopMatrix(1);

    g_depth++;
}

void glSpriteRotateScale(int x, int y, s32 angle, s32 scale, int flipmode,
                         const glImage *spr)
{
    int s_half_x = ((spr->width) + (spr->width & 1)) / 2;
    int s_half_y = ((spr->height) + (spr->height & 1)) / 2;

    int x1 = -s_half_x;
    int y1 = -s_half_y;

    int x2 = s_half_x;
    int y2 = s_half_y;

    int u1 = spr->u_off + ((flipmode & GL_FLIP_H) ? spr->width - 1 : 0);
    int u2 = spr->u_off + ((flipmode & GL_FLIP_H) ? 0 : spr->width - 1);
    int v1 = spr->v_off + ((flipmode & GL_FLIP_V) ? spr->height - 1 : 0);
    int v2 = spr->v_off + ((flipmode & GL_FLIP_V) ? 0 : spr->height - 1);

    if (spr->textureID != gCurrentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, spr->textureID);
        gCurrentTexture = spr->textureID;
    }

    glPushMatrix();

        gxTranslate3f32(x, y, 0);
        gxScalef32(scale, scale, 1 << 12);
        glRotateZi(angle);


        glBegin(GL_QUADS);

            gxTexcoord2i(u1, v1);
            gxVertex3i(x1, y1, g_depth);
            gxTexcoord2i(u1, v2);
            gxVertex2i(x1, y2);
            gxTexcoord2i(u2, v2);
            gxVertex2i(x2, y2);
            gxTexcoord2i(u2, v1);
            gxVertex2i(x2, y1);

        glEnd();

    glPopMatrix(1);

    g_depth++;
}

void glSpriteRotateScaleXY(int x, int y, s32 angle, s32 scaleX, s32 scaleY,
                           int flipmode, const glImage *spr)
{

    int s_half_x = ((spr->width) + (spr->width & 1)) / 2;
    int s_half_y = ((spr->height) + (spr->height & 1))  / 2;

    int x1 = -s_half_x;
    int y1 = -s_half_y;

    int x2 = s_half_x;
    int y2 = s_half_y;

    int u1 = spr->u_off + ((flipmode & GL_FLIP_H) ? spr->width - 1 : 0);
    int u2 = spr->u_off + ((flipmode & GL_FLIP_H) ? 0 : spr->width - 1);
    int v1 = spr->v_off + ((flipmode & GL_FLIP_V) ? spr->height - 1 : 0);
    int v2 = spr->v_off + ((flipmode & GL_FLIP_V) ? 0 : spr->height - 1);

    if (spr->textureID != gCurrentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, spr->textureID);
        gCurrentTexture = spr->textureID;
    }

    glPushMatrix();

        gxTranslate3f32(x, y, 0);
        gxScalef32(scaleX, scaleY, 1 << 12);
        glRotateZi(angle);


        glBegin(GL_QUADS);

            gxTexcoord2i(u1, v1);
            gxVertex3i(x1, y1, g_depth);
            gxTexcoord2i(u1, v2);
            gxVertex2i(x1, y2);
            gxTexcoord2i(u2, v2);
            gxVertex2i(x2, y2);
            gxTexcoord2i(u2, v1);
            gxVertex2i(x2, y1);

        glEnd();

    glPopMatrix(1);

    g_depth++;
}

void glSpriteStretchHorizontal(int x, int y, int length_x, const glImage *spr)
{
    int x1 = x;
    int y1 = y;
    int x2 = x + length_x;
    int y2 = y + spr->height;
    int su = (spr->width / 2) - 1;

    int u1 = spr->u_off;
    int u2 = spr->u_off + spr->width;
    int v1 = spr->v_off;
    int v2 = spr->v_off + spr->height;

    if (spr->textureID != gCurrentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, spr->textureID);
        gCurrentTexture = spr->textureID;
    }

    // left
    int x2l = x + su;
    glBegin(GL_QUADS);

        gxTexcoord2i(u1, v1);
        gxVertex3i(x1, y1, g_depth);

        gxTexcoord2i(u1, v2);
        gxVertex2i(x1, y2);

        gxTexcoord2i(u1 + su, v2);
        gxVertex2i(x2l, y2);

        gxTexcoord2i(u1 + su, v1);
        gxVertex2i(x2l, y1);

    glEnd();

    // center
    int x1l = x + su;
    x2l = x2 - su - 1;
    glBegin(GL_QUADS);

        gxTexcoord2i(u1 + su, v1);
        gxVertex2i(x1l, y1);

        gxTexcoord2i(u1 + su, v2);
        gxVertex2i(x1l, y2);

        gxTexcoord2i(u1 + su, v2);
        gxVertex2i(x2l, y2);

        gxTexcoord2i(u1 + su, v1);
        gxVertex2i(x2l, y1);

    glEnd();

    // right
    x1l = x2 - su - 1;
    glBegin(GL_QUADS);

        gxTexcoord2i(u1 + su, v1);
        gxVertex2i(x1l, y1);

        gxTexcoord2i(u1 + su, v2);
        gxVertex2i(x1l, y2);

        gxTexcoord2i(u2, v2);
        gxVertex2i(x2, y2);

        gxTexcoord2i(u2, v1);
        gxVertex2i(x2, y1);

    glEnd();

    g_depth++;
}

void glSpriteOnQuad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
                    int uoff, int voff, int flipmode, const glImage *spr)
{
    int u1 = spr->u_off + ((flipmode & GL_FLIP_H) ? spr->width - 1 : 0);
    int u2 = spr->u_off + ((flipmode & GL_FLIP_H) ? 0 : spr->width - 1);
    int v1 = spr->v_off + ((flipmode & GL_FLIP_V) ? spr->height - 1 : 0);
    int v2 = spr->v_off + ((flipmode & GL_FLIP_V) ? 0 : spr->height - 1);

    if (spr->textureID != gCurrentTexture)
    {
        glBindTexture(GL_TEXTURE_2D, spr->textureID);
        gCurrentTexture = spr->textureID;
    }

    glBegin(GL_QUADS);

        gxTexcoord2i(u1 + uoff, v1 + voff);
        gxVertex3i(x1, y1, g_depth);
        gxTexcoord2i(u1 + uoff, v2 + voff);
        gxVertex2i(x2, y2);
        gxTexcoord2i(u2 + uoff, v2 + voff);
        gxVertex2i(x3, y3);
        gxTexcoord2i(u2 + uoff, v1 + voff);
        gxVertex2i(x4, y4);

    glEnd();

    g_depth++;
}

int glLoadSpriteSet(glImage *sprite, const unsigned int numframes,
                    const unsigned int *texcoords, GL_TEXTURE_TYPE_ENUM type,
                    int sizeX, int sizeY, int param, int pallette_width,
                    const u16 *palette, const uint8_t *texture)
{
    int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(0, textureID);
    glTexImage2D(0, 0, type, sizeX, sizeY, 0, param, texture);
    glColorTableEXT(0, 0, pallette_width, 0, 0, palette);

    // Init sprites texture coords and texture ID
    for (unsigned int i = 0; i < numframes; i++)
    {
        int j = i * 4; // Texcoords array is u_off, wid, hei
        sprite[i].textureID = textureID;
        sprite[i].u_off = texcoords[j];      // set x-coord
        sprite[i].v_off = texcoords[j + 1];  // y-coord

        // Don't decrease because NDS 3d core does not draw last vertical texel
        sprite[i].width = texcoords[j + 2];
        sprite[i].height = texcoords[j + 3]; // Ditto
    }

    return textureID;
}

int glLoadTileSet(glImage *sprite, int tile_wid, int tile_hei, int bmp_wid, int bmp_hei,
                  GL_TEXTURE_TYPE_ENUM type, int sizeX, int sizeY, int param,
                  int pallette_width, const u16 *palette, const uint8_t *texture)
{
    int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(0, textureID);
    glTexImage2D(0, 0, type, sizeX, sizeY, 0, param, texture);
    glColorTableEXT(0, 0, pallette_width, 0, 0, palette);

    int i = 0;

    // Init sprites texture coords and texture ID
    for (int y = 0; y < (bmp_hei / tile_hei); y++)
    {
        for (int x = 0; x < (bmp_wid / tile_wid); x++)
        {
            sprite[i].width     = tile_wid;
            sprite[i].height    = tile_hei;
            sprite[i].u_off     = x * tile_wid;
            sprite[i].v_off     = y * tile_hei;
            sprite[i].textureID = textureID;
            i++;
        }
    }

    return textureID;
}
