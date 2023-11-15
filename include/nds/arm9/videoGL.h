// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

/// @file nds/arm9/videoGL.h
///
/// @brief Video API vaguely similar to OpenGL
///
/// For more information about the 3D hardware of the NDS, check GBATEK:
/// <A>https://www.problemkaputt.de/gbatek.htm#ds3dvideo</A>

#ifndef LIBNDS_NDS_ARM9_VIDEOGL_H__
#define LIBNDS_NDS_ARM9_VIDEOGL_H__

#include <nds/arm9/cache.h>
#include <nds/arm9/dynamicArray.h>
#include <nds/arm9/math.h>
#include <nds/arm9/sassert.h>
#include <nds/arm9/trig_lut.h>
#include <nds/arm9/video.h>
#include <nds/dma.h>
#include <nds/ndstypes.h>

#ifndef ARM9
#    error 3D hardware is only available from the ARM9
#endif

// Misc. constants

// This should be enough, but it can be changed.
#define MAX_TEXTURES 2048

// Fixed point / floating point / integer conversion macros

/// Depth in 12.3 fixed point.
///
/// Related functions: glClearDepth(), glCutoffDepth()
typedef uint16_t fixed12d3;

/// Convert int to fixed12d3
#define intto12d3(n)        ((n) << 3)
/// Convert float to fixed12d3
#define floatto12d3(n)      ((fixed12d3)((n) * (1 << 3)))
/// The maximum value for the type fixed12d3
#define GL_MAX_DEPTH        0x7FFF

/// Texture coordinate 12.4 in fixed point
typedef short t16;
/// Convert f32 to t16
#define f32tot16(n)         ((t16)(n >> 8))
/// Convert int to t16
#define inttot16(n)         ((n) << 4)
/// Convert t16 to int
#define t16toint(n)         ((n) >> 4)
/// Convert float to t16
#define floattot16(n)       ((t16)((n) * (1 << 4)))
/// Pack two t16 texture coordinate values into a 32 bit value
#define TEXTURE_PACK(u, v)  (((u) & 0xFFFF) | ((v) << 16))

/// Vertex coordinate in 4.12 fixed point
typedef short int v16;
/// Convert int to v16
#define inttov16(n)         ((n) << 12)
/// Convert f32 to v16
#define f32tov16(n)         (n)
/// Convert v16 to int
#define v16toint(n)         ((n) >> 12)
/// Convert float to v16
#define floattov16(n)       ((v16)((n) * (1 << 12)))
/// Pack two v16 values into one 32 bit value
#define VERTEX_PACK(x,y)    (u32)(((x) & 0xFFFF) | ((y) << 16))

/// Normal component in 0.10 fixed point, not used for 10 bit vertices.
typedef short int v10;
/// Convert int to v10
#define inttov10(n)         ((n) << 9)
/// Convert f32 to v10
#define f32tov10(n)         ((v10)((n) >> 3))
/// Convert v10 to int
#define v10toint(n)         ((n) >> 9)
/// Convert float to v10
#define floattov10(n)       (((n) > 0.998) ? 0x1FF : ((v10)((n) * (1 << 9))))
/// Pack 3 v10 normals into a 32 bit value
#define NORMAL_PACK(x,y,z)  (u32)(((x) & 0x3FF) | (((y) & 0x3FF) << 10) | ((z) << 20))

/// Holds a color value (1 bit alpha, 5 bits red, 5 bits green, 5 bits blue).
typedef unsigned short rgb;

/// Holds a 3x3 matrix
typedef struct m3x3 {
    int m[9]; ///< Array that holds the matrix
} m3x3;

/// Holds a 4x4 matrix
typedef struct m4x4 {
    int m[16]; ///< Array that holds the matrix
} m4x4;

/// Holds a 4x3 matrix
typedef struct m4x3 {
    int m[12]; ///< Array that holds the matrix
} m4x3;

/// Holds a vector.
///
/// Related functions: glScalev(), glTranslatev()
typedef struct GLvector {
    int x, y, z;
} GLvector;

#define GL_FALSE     0
#define GL_TRUE      1

/// Polygon drawing modes.
///
/// Related functions: glBegin()
typedef enum {
    /// Draw triangles with each 3 vertices defining a triangle.
    GL_TRIANGLES      = 0,
    /// Draw quads with each 4 vertices defining a quad.
    GL_QUADS          = 1,
    /// Draw triangles with the first triangle defined by 3 vertices and each
    /// additional triangle being defined by one additional vertex.
    GL_TRIANGLE_STRIP = 2,
    /// Draw quads with the first quad being defined by 4 vertices and each
    /// additional triangle being defined by 2 vertices.
    GL_QUAD_STRIP     = 3,
    /// Same as GL_TRIANGLES, old non-OpenGL version.
    GL_TRIANGLE       = 0,
    /// Same as GL_QUADS, old non-OpenGL version.
    GL_QUAD           = 1
} GL_GLBEGIN_ENUM;

/// Matrix modes.
///
/// Related functions: glMatrixMode()
typedef enum {
    GL_PROJECTION     = 0, ///< Projection matrix
    GL_POSITION       = 1, ///< Position matrix
    GL_MODELVIEW      = 2, ///< Modelview matrix
    GL_TEXTURE        = 3  ///< Texture matrix
} GL_MATRIX_MODE_ENUM;

/// Material types.
///
/// Related functions: glMaterialf()
typedef enum {
    /// Ambient color for the material (color when the normal is not facing the light).
    GL_AMBIENT             = 0x01,
    /// Diffuse color for the material (color when the normal is facing the light).
    GL_DIFFUSE             = 0x02,
    /// Ambient and diffuse colors for the material at the same time.
    GL_AMBIENT_AND_DIFFUSE = 0x03,
    /// Specular color for the material. The glossy (highlight) color of the polygon.
    GL_SPECULAR            = 0x04,
    /// Shininess color for the material. The color that shines back to the user.
    GL_SHININESS           = 0x08,
    /// Emission color for the material. Color independent of normals and lights.
    GL_EMISSION            = 0x10
} GL_MATERIALS_ENUM;

/// Polygon rendering attributes.
///
/// Related functions: glPolyFmt(), glInit(), POLY_ALPHA(), POLY_ID()
enum GL_POLY_FORMAT_ENUM {
    POLY_FORMAT_LIGHT0  = (1 << 0), ///< Enable light number 0
    POLY_FORMAT_LIGHT1  = (1 << 1), ///< Enable light number 1
    POLY_FORMAT_LIGHT2  = (1 << 2), ///< Enable light number 2
    POLY_FORMAT_LIGHT3  = (1 << 3), ///< Enable light number 3
    POLY_MODULATION     = (0 << 4), ///< Enable modulation shading mode (default)
    POLY_DECAL          = (1 << 4), ///< Enable decal shading
    POLY_TOON_HIGHLIGHT = (2 << 4), ///< Enable toon/highlight shading mode
    POLY_SHADOW         = (3 << 4), ///< Enable shadow shading
    POLY_CULL_FRONT     = (1 << 6), ///< Cull front polygons
    POLY_CULL_BACK      = (2 << 6), ///< Cull rear polygons
    POLY_CULL_NONE      = (3 << 6), ///< Don't cull any polygons
    POLY_FOG            = (1 << 15) ///< Enable/disable fog for this polygon
};

/// Possibles size of a texture (horizontal and vertical).
///
/// Related functions: glTexImage2d(), glTexParameter()
enum GL_TEXTURE_SIZE_ENUM {
    TEXTURE_SIZE_8    = 0, ///< 8 texels
    TEXTURE_SIZE_16   = 1, ///< 16 texels
    TEXTURE_SIZE_32   = 2, ///< 32 texels
    TEXTURE_SIZE_64   = 3, ///< 64 texels
    TEXTURE_SIZE_128  = 4, ///< 128 texels
    TEXTURE_SIZE_256  = 5, ///< 256 texels
    TEXTURE_SIZE_512  = 6, ///< 512 texels
    TEXTURE_SIZE_1024 = 7  ///< 1024 texels
};

/// Texture parameters such as texture wrapping and texture coord wrapping.
///
/// Related functions: glTexImage2d(), glTexParameter()
typedef enum  {
    /// Wrap (repeat) texture on S axis.
    GL_TEXTURE_WRAP_S = (1 << 16),
    /// Wrap (repeat) texture on T axis.
    GL_TEXTURE_WRAP_T = (1 << 17),
    /// Flip texture on S axis when wrapping.
    GL_TEXTURE_FLIP_S = (1 << 18),
    /// Flip texture on T axis when wrapping.
    GL_TEXTURE_FLIP_T = (1 << 19),
    /// Interpret color 0 as clear (same as old GL_TEXTURE_ALPHA_MASK).
    GL_TEXTURE_COLOR0_TRANSPARENT = (1 << 29),
    /// Use unmodified texture coordinates.
    TEXGEN_OFF      = (0 << 30),
    /// Multiply texture coordinates by the texture matrix.
    TEXGEN_TEXCOORD = (1 << 30),
    /// Set texture coordinates equal to normal * texture matrix, used for
    /// spherical reflection mapping.
    TEXGEN_NORMAL   = (int)(2U << 30),
    /// Set texture coordinates equal to vertex * texture matrix.
    TEXGEN_POSITION = (int)(3U << 30)
} GL_TEXTURE_PARAM_ENUM;

/// Texture formats.
///
/// Related functions: glTexImage2d(), glTexParameter()
typedef enum {
    GL_NOTEXTURE  = 0, ///< No texture is used - useful for making palettes
    GL_RGB32_A3   = 1, ///< 32 color palette, 3 bits of alpha
    GL_RGB4       = 2, ///< 4 color palette
    GL_RGB16      = 3, ///< 16 color palette
    GL_RGB256     = 4, ///< 256 color palette
    GL_COMPRESSED = 5, ///< Compressed texture
    GL_RGB8_A5    = 6, ///< 8 color palette, 5 bits of alpha
    GL_RGBA       = 7, ///< 15 bit direct color, 1 bit of alpha
    GL_RGB        = 8  ///< 15 bit direct color, manually sets alpha bit to 1
} GL_TEXTURE_TYPE_ENUM;

/// Enums for texture palette data retrieval
///
/// Related functions: glGetColorTableParameterEXT()
enum GL_TEXTURE_PALETTE_PARAM_ENUM {
    GL_COLOR_TABLE_FORMAT_EXT = 0, ///< Retrieve the palette address in memory
    GL_COLOR_TABLE_WIDTH_EXT  = 1  ///< Retrieve the size of the palette
};

/// 3D display control register bits.
///
/// Related functions: glEnable(), glDisable(), glInit()
enum DISP3DCNT_ENUM {
    /// Enable/disable textures on the geometry engine
    GL_TEXTURE_2D      = (1 << 0),
    /// Enable = Highlight shading; disable = Toon shading
    GL_TOON_HIGHLIGHT  = (1 << 1),
    /// Whether to use the alpha threshold set in glAlphaFunc()
    GL_ALPHA_TEST      = (1 << 2),
    /// Enable/disable alpha blending
    GL_BLEND           = (1 << 3),

    /// Enable/disable edge antialiasing.
    ///
    /// Antialiasing is applied to polygon edges, to make the edges look
    /// smoother. There is no antialiasing applied inside polygons or at
    /// intersections between two polygons. Antialiasing also doesn't apply to
    /// translucent pixels. Antialiasing interferes with wireframe polygons,
    /// lines, points, and edge marking.
    GL_ANTIALIAS       = (1 << 4),

    /// Enable/disable edge coloring; the high 3bits of the polygon ID
    /// determine the color; glSetOutlineColor() sets the available colors.
    GL_OUTLINE         = (1 << 5),
    /// If it's enabled, only the fog alpha value is used, not the color. If
    /// it's disabled, both fog color and alpha are used.
    GL_FOG_ONLY_ALPHA  = (1 << 6),
    /// Enables/disables fog
    GL_FOG             = (1 << 7),
    /// Enabled = color buffer underflow, setting it to 1 resets the overflow
    /// flag; disabled = no color buffer overflow.
    GL_COLOR_UNDERFLOW = (1 << 12),
    /// Enabled = polygon/vertex buffer overflow, setting it to 1 resets the
    /// overflow flag; disabled = no polygon/vertex buffer overflow.
    GL_POLY_OVERFLOW   = (1 << 13),
    /// Enable: rear/clear plane is in BMP mode; disable = rear/color plane is
    /// in clear mode.
    GL_CLEAR_BMP       = (1 << 14)
};

/// Enums for reading information from the geometry engine.
///
/// Related functions: glGetInt(), glGetFixed()
typedef enum {
    /// Returns a count of vertexes currently stored in hardware vertex ram. Use glGetInt()
    GL_GET_VERTEX_RAM_COUNT,
    /// Returns a count of polygons currently stored in hardware polygon ram. Use glGetInt()
    GL_GET_POLYGON_RAM_COUNT,
    /// Returns the current 3x3 directional vector matrix. Use glGetFixed()
    GL_GET_MATRIX_VECTOR,
    /// Returns the current 4x4 position matrix. Use glGetFixed()
    GL_GET_MATRIX_POSITION,
    /// Returns the current 4x4 projection matrix. Use glGetFixed()
    GL_GET_MATRIX_PROJECTION,
    /// Returns the current 4x4 clip matrix. Use glGetFixed()
    GL_GET_MATRIX_CLIP,
    /// Returns the width of the currently bound texture. Use glGetInt()
    GL_GET_TEXTURE_WIDTH,
    /// Returns the height of the currently bound texture. Use glGetInt()
    GL_GET_TEXTURE_HEIGHT
} GL_GET_ENUM;


/// Arguments for glFlush().
///
/// Related functions: glEnable(), glDisable(), glInit()
enum GLFLUSH_ENUM {
    /// Enable manual sorting of translucent polygons, otherwise uses Y-sorting
    GL_TRANS_MANUALSORT = (1 << 0),
    /// Enable W depth buffering of vertices, otherwise uses Z depth buffering
    GL_WBUFFERING       = (1 << 1)
};

// Structures specific to allocating and deallocating texture and palette VRAM
// ---------------------------------------------------------------------------

typedef struct s_SingleBlock {
    uint32_t indexOut;
    uint8_t *AddrSet;

    // 0-1: prev/next memory block
    // 2-3: prev/next empty/alloc block
    struct s_SingleBlock *node[4];

    uint32_t blockSize;
} s_SingleBlock;

typedef struct s_vramBlock {
    uint8_t *startAddr, *endAddr;
    struct s_SingleBlock *firstBlock;
    struct s_SingleBlock *firstEmpty;
    struct s_SingleBlock *firstAlloc;

    struct s_SingleBlock *lastExamined;
    uint8_t *lastExaminedAddr;
    uint32_t lastExaminedSize;

    DynamicArray blockPtrs;
    DynamicArray deallocBlocks;

    uint32_t blockCount;
    uint32_t deallocCount;
} s_vramBlock;

typedef struct gl_texture_data {
    void *vramAddr;       // Address to the texture loaded into VRAM
    uint32_t texIndex;    // The index in the Memory Block
    uint32_t texIndexExt; // The secondary index in the memory block (for GL_COMPRESSED)
    int palIndex;         // The palette index
    uint32_t texFormat;   // Specifications of how the texture is displayed
    uint32_t texSize;     // The size (in blocks) of the texture
} gl_texture_data;

typedef struct gl_palette_data {
    void *vramAddr;         // Address to the palette loaded into VRAM
    uint32_t palIndex;      // The index in the memory block
    uint16_t addr;          // The offset address for texture palettes in VRAM
    uint16_t palSize;       // The length of the palette
    uint32_t connectCount;  // The number of textures currently using this palette
} gl_palette_data;

// This struct hold hidden globals for videoGL. The structure is initialized in
// the .c file and returned by glGetGlobals() so that it can be used across
// compilation units without problem. This is automatically done by glInit() so
// don't worry too much about it. This is only an issue because of the mix of
// inlined/real functions.
typedef struct gl_hidden_globals {
    GL_MATRIX_MODE_ENUM matrixMode; // Holds the current Matrix Mode
    s_vramBlock *vramBlocks[2]; // One for textures and one for palettes
    int vramLock[2]; // Holds the current lock state of the VRAM banks

    // Texture globals
    DynamicArray texturePtrs; // Pointers to each individual texture
    DynamicArray palettePtrs; // Pointers to each individual palette

    DynamicArray deallocTex; // Preserves deleted names for later use with glGenTextures
    DynamicArray deallocPal; // Preserves deleted palette names
    uint32_t deallocTexSize; // Preserved number of deleted texture names
    uint32_t deallocPalSize; // Preserved number of deleted palette names

    int activeTexture; // The current active texture name
    int activePalette; // The current active palette name
    int texCount;
    int palCount;

    // Holds the current state of the clear color register
    u32 clearColor; // State of clear color register

    uint8_t isActive; // Has this been called before?
} gl_hidden_globals;

// Pointer to global data for videoGL
extern gl_hidden_globals glGlobalData;

// Pointer to global data for videoGL
static gl_hidden_globals *glGlob = &glGlobalData;

// FIFO commands
// -------------

/// Packs four packed commands into a 32bit command for sending to the GFX FIFO
#define FIFO_COMMAND_PACK(c1, c2, c3, c4) (((c4) << 24) | ((c3) << 16) | ((c2) << 8) | (c1))

/// Converts a GFX command for use in a packed command list
#define REG2ID(r)               (u8)((((u32)(&(r))) - 0x04000400) >> 2)

#define FIFO_NOP                REG2ID(GFX_FIFO)         ///< Nothing (padding)
#define FIFO_STATUS             REG2ID(GFX_STATUS)       ///< Geometry engine status register
#define FIFO_COLOR              REG2ID(GFX_COLOR)        ///< Direct vertex color

#define FIFO_VERTEX16           REG2ID(GFX_VERTEX16)     ///< Vertex with 3 16bit paramaters
#define FIFO_VERTEX10           REG2ID(GFX_VERTEX10)     ///< Vertex with 3 10bit paramaters
#define FIFO_VERTEX_XY          REG2ID(GFX_VERTEX_XY)    ///< Vertex that uses the last Z
#define FIFO_VERTEX_XZ          REG2ID(GFX_VERTEX_XZ)    ///< Vertex that uses the last Y
#define FIFO_VERTEX_YZ          REG2ID(GFX_VERTEX_YZ)    ///< Vertex that uses the last X
#define FIFO_TEX_COORD          REG2ID(GFX_TEX_COORD)    ///< Texture coordinate
#define FIFO_TEX_FORMAT         REG2ID(GFX_TEX_FORMAT)   ///< Texture format
#define FIFO_PAL_FORMAT         REG2ID(GFX_PAL_FORMAT)   ///< Texture palette attributes

#define FIFO_CLEAR_COLOR        REG2ID(GFX_CLEAR_COLOR)  ///< Clear color of the rear plane
#define FIFO_CLEAR_DEPTH        REG2ID(GFX_CLEAR_DEPTH)  ///< Depth of the rear plane

#define FIFO_LIGHT_VECTOR       REG2ID(GFX_LIGHT_VECTOR) ///< Direction of a light source
#define FIFO_LIGHT_COLOR        REG2ID(GFX_LIGHT_COLOR)  ///< Color for a light source
#define FIFO_NORMAL             REG2ID(GFX_NORMAL)       ///< Normal for following vertices

/// Diffuse and ambient material properties for the following vertices
#define FIFO_DIFFUSE_AMBIENT    REG2ID(GFX_DIFFUSE_AMBIENT)
/// Specular and emmissive material properties for the following vertices
#define FIFO_SPECULAR_EMISSION  REG2ID(GFX_SPECULAR_EMISSION)
/// Shininess table to be used for the following vertices
#define FIFO_SHININESS          REG2ID(GFX_SHININESS)

#define FIFO_POLY_FORMAT        REG2ID(GFX_POLY_FORMAT)  ///< Polygon attributes

#define FIFO_BEGIN              REG2ID(GFX_BEGIN)        ///< Starts a polygon vertex list
#define FIFO_END                REG2ID(GFX_END)          ///< Ends a polygon vertex list
#define FIFO_FLUSH              REG2ID(GFX_FLUSH)        ///< Flush the 3D context
#define FIFO_VIEWPORT           REG2ID(GFX_VIEWPORT)     ///< Set the viewport

#ifdef __cplusplus
extern "C" {
#endif

/// Rotates the model view matrix by angle about the specified unit vector.
///
/// @param angle The angle to rotate by
/// @param x X component of the unit vector axis.
/// @param y Y component of the unit vector axis.
/// @param z Z component of the unit vector axis.
void glRotatef32i(int angle, int32_t x, int32_t y, int32_t z);

/// Loads a 2D texture into texture memory and sets the currently bound texture
/// ID to the attributes specified.
///
/// @param target Ignored, only here for OpenGL compatibility.
/// @param empty1 Ignored, only here for OpenGL compatibility.
/// @param type The format of the texture.
/// @param sizeX the horizontal size of the texture, check GL_TEXTURE_SIZE_ENUM.
/// @param sizeY the vertical size of the texture, check GL_TEXTURE_SIZE_ENUM.
/// @param empty2 Ignored, only here for OpenGL compatibility.
/// @param param Parameters of the texture.
/// @param texture Pointer to the texture data to load.
/// @return 1 on success, 0 on failure.
int glTexImage2D(int target, int empty1, GL_TEXTURE_TYPE_ENUM type, int sizeX,
                 int sizeY, int empty2, int param, const void *texture);

/// Loads a 15-bit color palette into palette memory, and sets it to the
/// currently bound texture.
///
/// It can also remove palettes.
///
/// @param target Ignored, only here for OpenGL compatibility.
/// @param empty1 Ignored, only here for OpenGL compatibility.
/// @param width The length of the palette (if 0, the palette is removed from
///              currently bound texture).
/// @param empty2 Ignored, only here for OpenGL compatibility.
/// @param empty3 Ignored, only here for OpenGL compatibility.
/// @param table Pointer to the palette data to load (if NULL, the palette is
///              removed from currently bound texture).
void glColorTableEXT(int target, int empty1, uint16_t width, int empty2,
                     int empty3, const uint16_t *table);

/// Loads a 15-bit color format palette into a specific spot in a currently
/// bound texture's existing palette.
///
/// @param target Ignored, only here for OpenGL compatibility.
/// @param start The starting index that new palette data will be written to.
/// @param count The number of entries to write.
/// @param empty1 Ignored, only here for OpenGL compatibility.
/// @param empty2 Ignored, only here for OpenGL compatibility.
/// @param data Pointer to the palette data to load.
void glColorSubTableEXT(int target, int start, int count, int empty1,
                        int empty2, const uint16_t *data);

/// Retrieves a 15-bit color format palette from the palette memory of the
/// currently bound texture.
///
/// @param target Ignored, only here for OpenGL compatibility.
/// @param empty1 Ignored, only here for OpenGL compatibility.
/// @param empty2 Ignored, only here for OpenGL compatibility.
/// @param table Pointer where palette data will be written to.
void glGetColorTableEXT(int target, int empty1, int empty2, uint16_t *table);

/// glAssignColorTable sets the active texture with a palette set with another
/// texture.
///
/// @param target Ignored, only here for OpenGL compatibility.
/// @param name The name(int value) of the texture to load a palette from.
void glAssignColorTable(int target, int name);

/// Set parameters for the current texture.
///
/// Although it's named the same as its OpenGL counterpart, it is not
/// compatible.
///
/// @param target Ignored, only here for OpenGL compatibility.
/// @param param Paramaters for the texture.
void glTexParameter(int target, int param);

/// Returns the active texture parameter
///
/// @return The parameter.
u32 glGetTexParameter(void);

/// glGetColorTableParameterEXT retrieves information pertaining to the
/// currently bound texture's palette.
///
/// @param target Ignored, only here for OpenGL compatibility.
/// @param pname A parameter of type GL_TEXTURE_PALETTE_PARAM_ENUM, used to read
///              a specific attribute into params
/// @param params The destination for the attribute to read into.
void glGetColorTableParameterEXT(int target, int pname, int *params);

/// Returns the address alocated to the texure named by name.
///
/// @param name The name of the texture to get a pointer to.
/// @return The address.
void *glGetTexturePointer(int name);

/// glBindTexure sets the current named texture to the active texture.
///
/// The target is ignored as all DS textures are 2D.
///
/// @param target Ignored, only here for OpenGL compatibility.
/// @param name The name (int value) to set to the current texture.
void glBindTexture(int target, int name);

/// Creates room for the specified number of textures.
///
/// @param n The number of textures to generate.
/// @param names Pointer to the names array to fill.
/// @return 1 on success, 0 on failure.
int glGenTextures(int n, int *names);

/// Deletes the specified number of textures (and associated palettes).
///
/// @param n The number of textures to delete.
/// @param names Pointer to the names array to empty.
/// @return 1 on success, 0 on failure.
int glDeleteTextures(int n, int *names);

/// Resets the GL texture state freeing all texture and texture palette memory.
void glResetTextures(void);

/// Locks a designated VRAM bank to prevent consideration of the bank when
/// allocating textures.
///
/// @param addr The base address of the VRAM bank.
/// @return 1 on success, 0 on failure.
int glLockVRAMBank(uint16_t *addr);

/// Unlocks a designated VRAM bank to allow consideration of the bank when
/// allocating textures.
///
/// @param addr The base address of the VRAM bank.
/// @return 1 on success, 0 on failure.
int glUnlockVRAMBank(uint16_t *addr);

/// Sets texture coordinates for following vertices (fixed point version).
///
/// @param u U (a.k.a. S) texture coordinate (0.0 - 1.0).
/// @param v V (a.k.a. T) texture coordinate (0.0 - 1.0).
void glTexCoord2f32(int32_t u, int32_t v);

/// Specify the material properties to be used in rendering lit polygons.
///
/// @param mode Which material property to change.
/// @param color The color to set for that material property.
void glMaterialf(GL_MATERIALS_ENUM mode, rgb color);

// Private: Initializes the GL state.
int glInit_C(void);

// Private: This returns a pointer to the globals for videoGL
gl_hidden_globals* glGetGlobals(void);

#ifdef __cplusplus
}
#endif

/// Used in glPolyFmt() to set the alpha level for the following polygons.
///
/// Set to 0 for wireframe mode.
///
/// @param n The level of alpha (0 - 31).
/// @return Value to be used by glPolyFmt().
static inline u32 POLY_ALPHA(u32 n)
{
    return n << 16;
}

/// Used in glPolyFmt() to set the polygon ID for the following polygons.
///
/// @param n The ID to set for following polygons (0 - 63).
/// @return Value to be used by glPolyFmt().
static inline u32 POLY_ID(u32 n)
{
    return n << 24;
}

/// Starts a polygon group.
///
/// @param mode the draw mode for the polygon.
static inline void glBegin(GL_GLBEGIN_ENUM mode)
{
    GFX_BEGIN = mode;
}

/// Ends a polygon group.
static inline void glEnd(void)
{
    GFX_END = 0;
}

/// Reset the depth buffer to this value.
///
/// Generally set this to GL_MAX_DEPTH.
///
/// @param depth Distance from the camera. Generally set to GL_MAX_DEPTH.
static inline void glClearDepth(fixed12d3 depth)
{
    GFX_CLEAR_DEPTH = depth;
}

/// Set the color for the following vertices.
///
/// @param red The red component (0 - 255). Bottom 3 bits ignored.
/// @param green The green component (0 - 255). Bottom 3 bits ignored.
/// @param blue The blue component (0 - 255). Bottom 3 bits ignored.
static inline void glColor3b(uint8_t red, uint8_t green, uint8_t blue)
{
    GFX_COLOR = (vu32)RGB15(red >> 3, green >> 3, blue >> 3);
}

/// Set the color for the following vertices.
///
/// @param color The 15 bit color value.
static inline void glColor(rgb color)
{
    GFX_COLOR = (vu32)color;
}

/// Specifies a vertex.
///
/// @param x The x component for the vertex.
/// @param y The y component for the vertex.
/// @param z The z component for the vertex.
static inline void glVertex3v16(v16 x, v16 y, v16 z)
{
    GFX_VERTEX16 = (y << 16) | (x & 0xFFFF);
    GFX_VERTEX16 = z;
}

/// Sets texture coordinates for the following vertices.
///
/// @param u U (a.k.a. S) texture coordinate in texels.
/// @param v V (a.k.a. T) texture coordinate in texels.
static inline void glTexCoord2t16(t16 u, t16 v)
{
    GFX_TEX_COORD = TEXTURE_PACK(u, v);
}

/// Pushes the current matrix to the stack.
static inline void glPushMatrix(void)
{
    MATRIX_PUSH = 0;
}

/// Pops the specified number of matrices from the stack.
///
/// @param num The number of matrices to pop.
static inline void glPopMatrix(int num)
{
    MATRIX_POP = num;
}

/// Restores the current matrix from a location in the stack.
///
/// @param index The location in the stack.
static inline void glRestoreMatrix(int index)
{
    MATRIX_RESTORE = index;
}

/// Place the current matrix into the stack at the specified location.
///
/// @param index The location in the stack.
static inline void glStoreMatrix(int index)
{
    MATRIX_STORE = index;
}

/// Multiply the current matrix by a scale matrix.
///
/// @param v The vector to scale by.
static inline void glScalev(const GLvector *v)
{
    MATRIX_SCALE = v->x;
    MATRIX_SCALE = v->y;
    MATRIX_SCALE = v->z;
}

/// Multiply the current matrix by a translation matrix.
///
/// @param v The vector to translate by.
static inline void glTranslatev(const GLvector *v)
{
    MATRIX_TRANSLATE = v->x;
    MATRIX_TRANSLATE = v->y;
    MATRIX_TRANSLATE = v->z;
}

// Map old name to new name
#define glTranslate3f32 glTranslatef32

/// Multiply the current matrix by a translation matrix.
///
/// @param x Translation on the x axis.
/// @param y Translation on the y axis.
/// @param z Translation on the z axis.
static inline void glTranslatef32(int x, int y, int z)
{
    MATRIX_TRANSLATE = x;
    MATRIX_TRANSLATE = y;
    MATRIX_TRANSLATE = z;
}

/// Multiply the current matrix by a scale matrix.
///
/// @param x Scaling on the x axis.
/// @param y Scaling on the y axis.
/// @param z Scaling on the z axis.
static inline void glScalef32(int x, int y, int z)
{
    MATRIX_SCALE = x;
    MATRIX_SCALE = y;
    MATRIX_SCALE = z;
}

/// Set up a light.
///
/// Only parallel light sources are supported on the DS. Also, the direction
/// must be normalized.
///
/// @param id The number of the light to setup.
/// @param color The color of the light.
/// @param x X component of the lights directional vector.
/// @param y Y component of the lights directional vector.
/// @param z Z component of the lights directional vector.
static inline void glLight(int id, rgb color, v10 x, v10 y, v10 z)
{
    id = (id & 3) << 30;
    GFX_LIGHT_VECTOR = id | ((z & 0x3FF) << 20) | ((y & 0x3FF) << 10) | (x & 0x3FF);
    GFX_LIGHT_COLOR = id | color;
}

/// The normal to use for the following vertices.
///
/// @param normal the packed normal (three 10 bit values: x, y, z).
/// @warning The nature of the format means that you can't represent the
/// following normals exactly: (0,0,1), (0,1,0), (1,0,0)
static inline void glNormal(u32 normal)
{
    GFX_NORMAL = normal;
}

/// Loads an identity matrix to the current matrix, same as glIdentity().
static inline void glLoadIdentity(void)
{
    MATRIX_IDENTITY = 0;
}

/// Change the current matrix mode.
///
/// @param mode New mode for the matrix.
static inline void glMatrixMode(GL_MATRIX_MODE_ENUM mode)
{
    MATRIX_CONTROL = mode;
}

/// Specify the viewport for following drawing.
///
/// It can be set several times per frame.
///
/// @param x1 the left of the viewport.
/// @param y1 the bottom of the viewport.
/// @param x2 the right of the viewport.
/// @param y2 the top of the viewport.
static inline void glViewport(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    GFX_VIEWPORT = x1 + (y1 << 8) + (x2 << 16) + (y2 << 24);
}

/// Waits for a vertical blank (like swiWaitForVBlank) and swaps the buffers.
///
/// It lets you specify some 3D options: enabling Y-sorting of translucent
/// polygons and W-Buffering of all vertices.
///
/// @param mode Flags from GLFLUSH_ENUM.
static inline void glFlush(u32 mode)
{
    // Compiler barrier to prevent reordering of commands before GFX_FLUSH
    asm volatile("" ::: "memory");
    GFX_FLUSH = mode;
}

/// The DS uses a table for shininess. This generates one.
static inline void glMaterialShinyness(void)
{
    u32 shiny32[128 / 4];
    u8 *shiny8 = (u8 *)shiny32;

    for (int i = 0; i < 128 * 2; i += 2)
        shiny8[i >> 1] = i;

    for (int i = 0; i < 128 / 4; i++)
        GFX_SHININESS = shiny32[i];
}

/// Sends a packed list of commands into the graphics FIFO via asyncronous DMA.
///
/// The first 32 bits is the length of the packed command list, followed by the
/// packed list.
///
/// @param list Pointer to the packed list.
static inline void glCallList(const u32 *list)
{
    sassert(list != NULL, "glCallList received a null display list pointer");

    u32 count = *list++;

    sassert(count != 0, "glCallList received a display list of size 0");

    // Flush the area that we are going to DMA
    DC_FlushRange(list, count * 4);

    // There is a hardware bug that affects DMA when there are multiple channels
    // active, under certain conditions. Instead of checking for said
    // conditions, simply ensure that there are no DMA channels active.
    while (dmaBusy(0) || dmaBusy(1) || dmaBusy(2) || dmaBusy(3));

    // Send the packed list asynchronously via DMA to the FIFO
    DMA_SRC(0) = (uint32_t)list;
    DMA_DEST(0) = (uint32_t)&GFX_FIFO;
    DMA_CR(0) = DMA_FIFO | count;
    while (dmaBusy(0));
}

/// Set the parameters for polygons rendered on the current frame.
///
/// Valid paramters are enumerated in GL_POLY_FORMAT_ENUM and in the functions
/// POLY_ALPHA() and POLY_ID().
///
/// @param params The paramters to set for the following polygons.
static inline void glPolyFmt(u32 params)
{
    GFX_POLY_FORMAT = params;
}

/// Enables various GL states (blend, alpha test, etc..).
///
/// @param bits Bit mask of desired attributes, enumerated in DISP3DCNT_ENUM.
static inline void glEnable(int bits)
{
    GFX_CONTROL |= bits;
}

/// Disables various GL states (blend, alpha test, etc..).
///
/// @param bits Bit mask of desired attributes, enumerated in DISP3DCNT_ENUM.
static inline void glDisable(int bits)
{
    GFX_CONTROL &= ~bits;
}

/// Sets the FOG_SHIFT value
///
/// Each entry of the fog table covers 0x400 >> FOG_SHIFT depth values.
///
/// @param shift FOG_SHIFT value.
static inline void glFogShift(int shift)
{
    sassert(shift >= 0 && shift < 16, "glFogShift is out of range");

    GFX_CONTROL = (GFX_CONTROL & 0xF0FF) | (shift << 8);
}

/// Sets the FOG_OFFSET value.
///
/// Fog begins at this depth with a density of FOG_TABLE[0].
///
/// @param offset FOG_OFFSET value.
static inline void glFogOffset(int offset)
{
    sassert(offset >= 0 && offset < 0x8000, "glFogOffset is out of range");

    GFX_FOG_OFFSET = offset;
}

/// Sets the fog color.
///
/// @param red Red component (0 - 31).
/// @param green Green component (0 - 31).
/// @param blue Blue component (0 - 31).
/// @param alpha From 0 (clear) to 31 (opaque).
static inline void glFogColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    sassert(red < 32, "glFogColor red is out of range");
    sassert(green < 32, "glFogColor green is out of range");
    sassert(blue < 32, "glFogColor blue is out of range");
    sassert(alpha < 32, "glFogColor alpha is out of range");

    GFX_FOG_COLOR = RGB15(red, green, blue) | (alpha << 16);
}

/// Sets the fog density at a given index.
///
/// @param index Fog table index to operate on (0 to 31).
/// @param density Fog density from 0 (none) to 127 (opaque).
static inline void glFogDensity(int index, int density)
{
    sassert(index >= 0 && index < 32, "glFogDensity index is out of range");
    sassert(index >= 0 && density < 128, "glFogDensity density is out of range");

    GFX_FOG_TABLE[index] = density;
}

/// Loads a 4x4 matrix into the current matrix.
///
/// @param m Pointer to a 4x4 matrix.
static inline void glLoadMatrix4x4(const m4x4 *m)
{
    MATRIX_LOAD4x4 = m->m[0];
    MATRIX_LOAD4x4 = m->m[1];
    MATRIX_LOAD4x4 = m->m[2];
    MATRIX_LOAD4x4 = m->m[3];

    MATRIX_LOAD4x4 = m->m[4];
    MATRIX_LOAD4x4 = m->m[5];
    MATRIX_LOAD4x4 = m->m[6];
    MATRIX_LOAD4x4 = m->m[7];

    MATRIX_LOAD4x4 = m->m[8];
    MATRIX_LOAD4x4 = m->m[9];
    MATRIX_LOAD4x4 = m->m[10];
    MATRIX_LOAD4x4 = m->m[11];

    MATRIX_LOAD4x4 = m->m[12];
    MATRIX_LOAD4x4 = m->m[13];
    MATRIX_LOAD4x4 = m->m[14];
    MATRIX_LOAD4x4 = m->m[15];
}

/// Loads a 4x3 matrix into the current matrix.
///
/// @param m Pointer to a 4x3 matrix.
static inline void glLoadMatrix4x3(const m4x3 *m)
{
    MATRIX_LOAD4x3 = m->m[0];
    MATRIX_LOAD4x3 = m->m[1];
    MATRIX_LOAD4x3 = m->m[2];
    MATRIX_LOAD4x3 = m->m[3];

    MATRIX_LOAD4x3 = m->m[4];
    MATRIX_LOAD4x3 = m->m[5];
    MATRIX_LOAD4x3 = m->m[6];
    MATRIX_LOAD4x3 = m->m[7];

    MATRIX_LOAD4x3 = m->m[8];
    MATRIX_LOAD4x3 = m->m[9];
    MATRIX_LOAD4x3 = m->m[10];
    MATRIX_LOAD4x3 = m->m[11];
}

/// Multiplies the current matrix by a 4x4 matrix.
///
/// @param m Pointer to a 4x4 matrix.
static inline void glMultMatrix4x4(const m4x4 *m)
{
    MATRIX_MULT4x4 = m->m[0];
    MATRIX_MULT4x4 = m->m[1];
    MATRIX_MULT4x4 = m->m[2];
    MATRIX_MULT4x4 = m->m[3];

    MATRIX_MULT4x4 = m->m[4];
    MATRIX_MULT4x4 = m->m[5];
    MATRIX_MULT4x4 = m->m[6];
    MATRIX_MULT4x4 = m->m[7];

    MATRIX_MULT4x4 = m->m[8];
    MATRIX_MULT4x4 = m->m[9];
    MATRIX_MULT4x4 = m->m[10];
    MATRIX_MULT4x4 = m->m[11];

    MATRIX_MULT4x4 = m->m[12];
    MATRIX_MULT4x4 = m->m[13];
    MATRIX_MULT4x4 = m->m[14];
    MATRIX_MULT4x4 = m->m[15];
}

/// Multiplies the current matrix by a 4x3 matrix.
///
/// @param m Pointer to a 4x3 matrix.
static inline void glMultMatrix4x3(const m4x3 *m)
{
    MATRIX_MULT4x3 = m->m[0];
    MATRIX_MULT4x3 = m->m[1];
    MATRIX_MULT4x3 = m->m[2];
    MATRIX_MULT4x3 = m->m[3];

    MATRIX_MULT4x3 = m->m[4];
    MATRIX_MULT4x3 = m->m[5];
    MATRIX_MULT4x3 = m->m[6];
    MATRIX_MULT4x3 = m->m[7];

    MATRIX_MULT4x3 = m->m[8];
    MATRIX_MULT4x3 = m->m[9];
    MATRIX_MULT4x3 = m->m[10];
    MATRIX_MULT4x3 = m->m[11];
}

/// Multiplies the current matrix by a 3x3 matrix.
///
/// @param m Pointer to a 3x3 matrix.
static inline void glMultMatrix3x3(const m3x3 *m)
{
    MATRIX_MULT3x3 = m->m[0];
    MATRIX_MULT3x3 = m->m[1];
    MATRIX_MULT3x3 = m->m[2];

    MATRIX_MULT3x3 = m->m[3];
    MATRIX_MULT3x3 = m->m[4];
    MATRIX_MULT3x3 = m->m[5];

    MATRIX_MULT3x3 = m->m[6];
    MATRIX_MULT3x3 = m->m[7];
    MATRIX_MULT3x3 = m->m[8];
}

/// Rotates the current modelview matrix by angle around the X axis.
///
/// @param angle The angle to rotate by (angle is -32768 to 32767).
static inline void glRotateXi(int angle)
{
    int sine = sinLerp(angle);
    int cosine = cosLerp(angle);

    MATRIX_MULT3x3 = inttof32(1);
    MATRIX_MULT3x3 = 0;
    MATRIX_MULT3x3 = 0;

    MATRIX_MULT3x3 = 0;
    MATRIX_MULT3x3 = cosine;
    MATRIX_MULT3x3 = sine;

    MATRIX_MULT3x3 = 0;
    MATRIX_MULT3x3 = -sine;
    MATRIX_MULT3x3 = cosine;
}

/// Rotates the current modelview matrix by angle around the Y axis.
///
/// @param angle The angle to rotate by (angle is -32768 to 32767).
static inline void glRotateYi(int angle)
{
    int sine = sinLerp(angle);
    int cosine = cosLerp(angle);

    MATRIX_MULT3x3 = cosine;
    MATRIX_MULT3x3 = 0;
    MATRIX_MULT3x3 = -sine;

    MATRIX_MULT3x3 = 0;
    MATRIX_MULT3x3 = inttof32(1);
    MATRIX_MULT3x3 = 0;

    MATRIX_MULT3x3 = sine;
    MATRIX_MULT3x3 = 0;
    MATRIX_MULT3x3 = cosine;
}

/// Rotates the current modelview matrix by angle around the Z axis.
///
/// @param angle The angle to rotate by (angle is -32768 to 32767).
static inline void glRotateZi(int angle)
{
    int sine = sinLerp(angle);
    int cosine = cosLerp(angle);

    MATRIX_MULT3x3 = cosine;
    MATRIX_MULT3x3 = sine;
    MATRIX_MULT3x3 = 0;

    MATRIX_MULT3x3 = -sine;
    MATRIX_MULT3x3 = cosine;
    MATRIX_MULT3x3 = 0;

    MATRIX_MULT3x3 = 0;
    MATRIX_MULT3x3 = 0;
    MATRIX_MULT3x3 = inttof32(1);
}

/// Multiplies the current matrix into orthographic mode.
///
/// @param left Left vertical clipping plane.
/// @param right Right vertical clipping plane.
/// @param bottom Bottom vertical clipping plane.
/// @param top Top vertical clipping plane.
/// @param zNear Near clipping plane.
/// @param zFar Far clipping plane.
static inline void glOrthof32(int left, int right, int bottom, int top,
                              int zNear, int zFar)
{
    MATRIX_MULT4x4 = divf32(inttof32(2), right - left);
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;

    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = divf32(inttof32(2), top - bottom);
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;

    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = divf32(inttof32(-2), zFar - zNear);
    MATRIX_MULT4x4 = 0;

    MATRIX_MULT4x4 = -divf32(right + left, right - left);
    MATRIX_MULT4x4 = -divf32(top + bottom, top - bottom);
    MATRIX_MULT4x4 = -divf32(zFar + zNear, zFar - zNear);
    MATRIX_MULT4x4 = floattof32(1.0f);
}

/// Places the camera at the specified location and orientation (fixed point
/// version).
///
/// @param eyex (eyex, eyey, eyez) Location of the camera.
/// @param eyey (eyex, eyey, eyez) Location of the camera.
/// @param eyez (eyex, eyey, eyez) Location of the camera.
/// @param lookAtx (lookAtx, lookAty, lookAtz) Where the camera is looking.
/// @param lookAty (lookAtx, lookAty, lookAtz) Where the camera is looking.
/// @param lookAtz (lookAtx, lookAty, lookAtz) Where the camera is looking.
/// @param upx <upx, upy, upz> Unit vector describing which direction is up for
///            the camera.
/// @param upy <upx, upy, upz> Unit vector describing which direction is up for
///            the camera.
/// @param upz <upx, upy, upz> Unit vector describing which direction is up for
///            the camera.
static inline void gluLookAtf32(int eyex, int eyey, int eyez,
                                int lookAtx, int lookAty, int lookAtz,
                                int upx, int upy, int upz)
{
    int32_t side[3], forward[3], up[3], eye[3];

    forward[0] = eyex - lookAtx;
    forward[1] = eyey - lookAty;
    forward[2] = eyez - lookAtz;

    normalizef32(forward);

    up[0] = upx;
    up[1] = upy;
    up[2] = upz;
    eye[0] = eyex;
    eye[1] = eyey;
    eye[2] = eyez;

    crossf32(up, forward, side);

    normalizef32(side);

    // Recompute local up
    crossf32(forward, side, up);

    glMatrixMode(GL_MODELVIEW);

    MATRIX_MULT4x3 = side[0];
    MATRIX_MULT4x3 = up[0];
    MATRIX_MULT4x3 = forward[0];

    MATRIX_MULT4x3 = side[1];
    MATRIX_MULT4x3 = up[1];
    MATRIX_MULT4x3 = forward[1];

    MATRIX_MULT4x3 = side[2];
    MATRIX_MULT4x3 = up[2];
    MATRIX_MULT4x3 = forward[2];

    MATRIX_MULT4x3 = -dotf32(eye,side);
    MATRIX_MULT4x3 = -dotf32(eye,up);
    MATRIX_MULT4x3 = -dotf32(eye,forward);
}

/// Specifies the viewing frustum for the projection matrix (fixed point
/// version).
///
/// @param left Left of a rectangle located at the near clipping plane.
/// @param right Right of a rectangle located at the near clipping plane.
/// @param top top of a rectangle located at the near clipping plane.
/// @param bottom Bottom of a rectangle located at the near clipping plane.
/// @param near Location of a the near clipping plane (parallel to viewing
///             window).
/// @param far Location of a the far clipping plane (parallel to viewing
///            window).
static inline void glFrustumf32(int left, int right, int bottom, int top,
                                int near, int far)
{
    MATRIX_MULT4x4 = divf32(2 * near, right - left);
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;

    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = divf32(2 * near, top - bottom);
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;

    MATRIX_MULT4x4 = divf32(right + left, right - left);
    MATRIX_MULT4x4 = divf32(top + bottom, top - bottom);
    MATRIX_MULT4x4 = -divf32(far + near, far - near);
    MATRIX_MULT4x4 = floattof32(-1.0f);

    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = -divf32(2 * mulf32(far, near), far - near);
    MATRIX_MULT4x4 = 0;
}

/// Utility function which sets up the projection matrix (fixed point version).
///
/// @param fovy Specifies the field of view in degrees (-32768 to 32767).
/// @param aspect Specifies the aspect ratio of the screen (normally screen
///               width divided by screen height).
/// @param zNear Specifies the near clipping plane.
/// @param zFar Specifies the far clipping plane.
static inline void gluPerspectivef32(int fovy, int aspect, int zNear, int zFar)
{
    int xmin, xmax, ymin, ymax;

    ymax = mulf32(zNear, tanLerp(fovy >> 1));

    ymin = -ymax;
    xmin = mulf32(ymin, aspect);
    xmax = mulf32(ymax, aspect);

    glFrustumf32(xmin, xmax, ymin, ymax, zNear, zFar);
}

/// Utility function which generates a picking matrix for selection.
///
/// @param x 2D x of center (touch x normally).
/// @param y 2D y of center (touch y normally).
/// @param width Width in pixels of the window (3 or 4 is a good number).
/// @param height Height in pixels of the window (3 or 4 is a good number).
/// @param viewport The current viewport (normally {0, 0, 255, 191}).
static inline void gluPickMatrix(int x, int y, int width, int height,
                                 const int viewport[4])
{
    MATRIX_MULT4x4 = inttof32(viewport[2]) / width;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = inttof32(viewport[3]) / height;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = inttof32(1);
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = inttof32(viewport[2] + ((viewport[0] - x) << 1)) / width;
    MATRIX_MULT4x4 = inttof32(viewport[3] + ((viewport[1] - y) << 1)) / height;
    MATRIX_MULT4x4 = 0;
    MATRIX_MULT4x4 = inttof32(1);
}

/// Resets matrix stack to top level
static inline void glResetMatrixStack(void)
{
    // Make sure there are no push/pops that haven't executed yet
    while(GFX_STATUS & GFX_STATUS_MATRIX_STACK_BUSY)
    {
        // Clear push/pop errors or push/pop busy bit never clears
        GFX_STATUS |= GFX_STATUS_MATRIX_STACK_ERROR;
    }

    // Pop the projection stack to the top; poping 0 off an empty stack causes
    // an error.
    if ((GFX_STATUS & (1 << 13)) != 0)
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix(1);
    }

    // 31 deep modelview matrix; 32nd entry works but sets error flag
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix((GFX_STATUS >> 8) & 0x1F);

    // Load identity to all the matrices
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
}

/// Specifies an edge color for polygons.
///
/// @param id Which outline color to set (0 - 7).
/// @param color The 15 bit color to set
static inline void glSetOutlineColor(int id, rgb color)
{
    GFX_EDGE_TABLE[id] = color;
}

/// Loads a toon table.
///
/// @param table Pointer to the 32 color palette to load into the toon table.
static inline void glSetToonTable(const uint16_t *table)
{
    for (int i = 0; i < 32; i++)
        GFX_TOON_TABLE[i] = table[i];
}

/// Sets a range of colors on the toon table.
///
/// @param start the start of the range
/// @param end the end of the range
/// @param color the color to set for that range */
static inline void glSetToonTableRange(int start, int end, rgb color)
{
    for (int i = start; i <= end; i++)
        GFX_TOON_TABLE[i] = color;
}

/// Gets fixed format of state variables.
///
/// OpenGL's modelview matrix is handled on the DS with two matrices. The
/// combination of the DS's position matrix and directional vector matrix holds
/// the data that is in OpenGL's one modelview matrix. (a.k.a. modelview =
/// postion and vector).
///
/// @param param The state variable to retrieve.
/// @param f Pointer with room to hold the requested data.
void glGetFixed(const GL_GET_ENUM param, int *f);

/// Set the minimum alpha value that will be displayed.
///
/// Polygons with a lower alpha value won't be displayed.
///
/// @param alphaThreshold Minimum alpha value that will be used (0 - 15).
static inline void glAlphaFunc(int alphaThreshold)
{
    GFX_ALPHA_TEST = alphaThreshold;
}

/// Stop the drawing of polygons that are a certain distance from the camera.
///
/// Polygons that are beyond this W-value (distance from camera) will not be
/// drawn.
///
/// @param wVal Distance (15 bit value).
static inline void glCutoffDepth(fixed12d3 wVal)
{
    GFX_CUTOFF_DEPTH = wVal;
}

/// Initializes the GL state machine (must be called once before using GL
/// calls).
///
/// @return 1 on success, 0 on failure
static inline int glInit(void)
{
    return glInit_C(); // Actually does the initialization
}

/// Sets the color of the rear-plane (a.k.a clear color/plane)
///
/// @param red Red component (0 - 31).
/// @param green Green component (0 - 31).
/// @param blue Blue component (0 - 31).
/// @param alpha Alpha from 0 (clear) to 31 (opaque).
static inline void glClearColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    glGlob->clearColor = (glGlob->clearColor & 0xFFE08000)
                       | (0x7FFF & RGB15(red, green, blue))
                       | ((alpha & 0x1F) << 16);
    GFX_CLEAR_COLOR = glGlob->clearColor;
}

/// Sets the polygon ID of the rear-plane (a.k.a. clear color/plane)
///
/// Useful for antialiasing and edge coloring.
///
/// @param ID The polygon ID to give the rear-plane.
static inline void glClearPolyID(uint8_t ID)
{
    glGlob->clearColor = (glGlob->clearColor & 0xC0FFFFFF) | ((ID & 0x3F) << 24);
    GFX_CLEAR_COLOR = glGlob->clearColor;
}

/// Grabs integer state variables from OpenGL.
///
/// @param param The state variable to retrieve
/// @param i Pointer with room to hold the requested data
void glGetInt(GL_GET_ENUM param, int *i);

/// Specifies a vertex location.
///
/// @param x The x component of the vertex.
/// @param y The y component of the vertex.
/// @param z The z component of the vertex.
/// @warning Float version! Please, use glVertex3v16() instead.
static inline void glVertex3f(float x, float y, float z)
{
    glVertex3v16(floattov16(x), floattov16(y), floattov16(z));
}

/// Rotate on an arbitrary axis.
///
/// @param angle The angle to rotate by
/// @param x The x component of the axis to rotate on.
/// @param y The y component of the axis to rotate on.
/// @param z The z component of the axis to rotate on.
/// @warning Float version! Please, use glRotatef32i() instead.
static inline void glRotatef32(float angle, int x, int y, int z)
{
    glRotatef32i((int)(angle * DEGREES_IN_CIRCLE / 360.0), x, y, z);
}

/// Rotate about an arbitrary axis.
///
/// @param x The x component of the axis to rotate on.
/// @param y The y component of the axis to rotate on.
/// @param z The z component of the axis to rotate on.
/// @param angle The angle to rotate by.
static inline void glRotatef(float angle, float x, float y, float z)
{
    glRotatef32(angle, floattof32(x), floattof32(y), floattof32(z));
}

/// Specify a color for following vertices.
///
/// @param r The red component of the color.
/// @param g The green component of the color.
/// @param b The blue component of the color.
/// @warning Float version! Please, use glColor3b() instead.
static inline void glColor3f(float r, float g, float b)
{
    glColor3b((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

/// Multiply the current matrix by a scale matrix.
///
/// @param x Scaling on the x axis.
/// @param y Scaling on the y axis.
/// @param z Scaling on the z axis.
/// @warning Float version! Please, use glScalev() or glScalef32() instead.
static inline void glScalef(float x, float y, float z)
{
    MATRIX_SCALE = floattof32(x);
    MATRIX_SCALE = floattof32(y);
    MATRIX_SCALE = floattof32(z);
}

/// Multiply the current matrix by a translation matrix.
///
/// @param x Translation on the x axis.
/// @param y Translation on the y axis.
/// @param z Translation on the z axis.
/// @warning Float version! Please, use glTranslatef32() instead.
static inline void glTranslatef(float x, float y, float z)
{
    MATRIX_TRANSLATE = floattof32(x);
    MATRIX_TRANSLATE = floattof32(y);
    MATRIX_TRANSLATE = floattof32(z);
}

/// The normal to use for following vertices.
///
/// @param x X component of the normal, vector must be normalized.
/// @param y Y component of the normal, vector must be normalized.
/// @param z Z component of the normal, vector must be normalized.
/// @warning Float version! Please use glNormal() instead.
static inline void glNormal3f(float x, float y, float z)
{
    glNormal(NORMAL_PACK(floattov10(x), floattov10(y), floattov10(z)));
}

/// Rotates the current modelview matrix by angle degrees around the X axis.
///
/// @param angle The angle to rotate by.
/// @warning Float version! Please, use glRotateZi() instead.
static inline void glRotateX(float angle)
{
    glRotateXi((int)(angle * DEGREES_IN_CIRCLE / 360.0));
}

/// Rotates the current modelview matrix by angle degrees around the Y axis.
///
/// @param angle The angle to rotate by.
/// @warning Float version! Please, use glRotateZi() instead.
static inline void glRotateY(float angle)
{
    glRotateYi((int)(angle * DEGREES_IN_CIRCLE / 360.0));
}

/// Rotates the current modelview matrix by angle degrees around the Z axis.
///
/// @param angle The angle to rotate by.
/// @warning Float version! Please, use glRotateZi() instead.
static inline void glRotateZ(float angle)
{
    glRotateZi((int)(angle * DEGREES_IN_CIRCLE / 360.0));
}

/// Multiplies the current matrix into ortho graphic mode.
///
/// @param left Left vertical clipping plane.
/// @param right Right vertical clipping plane.
/// @param bottom Bottom vertical clipping plane.
/// @param top Top vertical clipping plane.
/// @param zNear Near clipping plane.
/// @param zFar Far clipping plane.
/// @warning Float version! Please, use glOrthof32() instead.
static inline void glOrtho(float left, float right, float bottom, float top,
                           float zNear, float zFar)
{
    glOrthof32(floattof32(left), floattof32(right), floattof32(bottom),
               floattof32(top), floattof32(zNear), floattof32(zFar));
}

/// Places the camera at the specified location and orientation (floating point
/// version).
///
/// @param eyex (eyex, eyey, eyez) Location of the camera.
/// @param eyey (eyex, eyey, eyez) Location of the camera.
/// @param eyez (eyex, eyey, eyez) Location of the camera.
/// @param lookAtx (lookAtx, lookAty, lookAtz) Where the camera is looking.
/// @param lookAty (lookAtx, lookAty, lookAtz) Where the camera is looking.
/// @param lookAtz (lookAtx, lookAty, lookAtz) Where the camera is looking.
/// @param upx <upx, upy, upz> Unit vector describing which direction is up for
///            the camera.
/// @param upy <upx, upy, upz> Unit vector describing which direction is up for
///            the camera.
/// @param upz <upx, upy, upz> Unit vector describing which direction is up for
///            the camera.
/// @warning Float version! Please, use gluLookAtf32() instead.
static inline void gluLookAt(float eyex, float eyey, float eyez,
                             float lookAtx, float lookAty, float lookAtz,
                             float upx, float upy, float upz)
{
    gluLookAtf32(floattof32(eyex), floattof32(eyey), floattof32(eyez),
                 floattof32(lookAtx), floattof32(lookAty), floattof32(lookAtz),
                 floattof32(upx), floattof32(upy), floattof32(upz));
}

/// Specifies the viewing frustum for the projection matrix (floating point
/// version).
///
/// @param left Left of a rectangle located at the near clipping plane.
/// @param right Right of a rectangle located at the near clipping plane.
/// @param top Top of a rectangle located at the near clipping plane.
/// @param bottom Bottom of a rectangle located at the near clipping plane.
/// @param near Location of a the near clipping plane (parallel to viewing
///             window).
/// @param far Location of a the far clipping plane (parallel to viewing
///            window).
/// @warning Float version! Please, use glFrustumf32() instead.
static inline void glFrustum(float left, float right, float bottom, float top,
                             float near, float far)
{
    glFrustumf32(floattof32(left), floattof32(right), floattof32(bottom),
                 floattof32(top), floattof32(near), floattof32(far));
}

/// Utility function that sets up the projection matrix (floating point version)
///
/// @param fovy Specifies the field of view in degrees.
/// @param aspect Specifies the aspect ratio of the screen (normally screen
///               width/screen height).
/// @param zNear Specifies the near clipping plane.
/// @param zFar Specifies the far clipping plane.
/// @warning Float version! Please, use gluPerspectivef32() instead.
static inline void gluPerspective(float fovy, float aspect, float zNear, float zFar)
{
    gluPerspectivef32((int)(fovy * DEGREES_IN_CIRCLE / 360.0), floattof32(aspect),
                      floattof32(zNear), floattof32(zFar));
}

/// Sets texture coordinates for following vertices
///
/// @param s S (a.k.a. U) texture coordinate (0.0 - 1.0).
/// @param t T (a.k.a. V) texture coordinate (0.0 - 1.0).
/// @warning Float version! Please, use glTexCoord2t16() instead.
void glTexCoord2f(float s, float t);

#endif // LIBNDS_NDS_ARM9_VIDEOGL_H__
