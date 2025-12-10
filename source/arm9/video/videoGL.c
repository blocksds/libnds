// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Video API vaguely similar to OpenGL

#include <nds/arm9/math.h>
#include <nds/arm9/sassert.h>
#include <nds/arm9/trig_lut.h>
#include <nds/arm9/videoGL.h>
#include <nds/arm9/video.h>
#include <nds/bios.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

// Structures specific to allocating and deallocating texture and palette VRAM
// ---------------------------------------------------------------------------

typedef struct s_SingleBlock
{
    uint32_t indexOut;
    uint8_t *AddrSet;

    // 0-1: prev/next memory block
    // 2-3: prev/next empty/alloc block
    struct s_SingleBlock *node[4];

    uint32_t blockSize;
} s_SingleBlock;

typedef struct s_vramBlock
{
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

typedef struct gl_texture_data
{
    void *vramAddr;       // Address to the texture loaded into VRAM
    uint32_t texIndex;    // The index in the Memory Block
    uint32_t texIndexExt; // The secondary index in the memory block (for GL_COMPRESSED)
    int palIndex;         // The palette index
    uint32_t texFormat;   // Specifications of how the texture is displayed
    uint32_t texSize;     // The size (in blocks) of the texture
} gl_texture_data;

typedef struct gl_palette_data
{
    void *vramAddr;         // Address to the palette loaded into VRAM
    uint32_t palIndex;      // The index in the memory block
    uint16_t addr;          // The offset address for texture palettes in VRAM
    uint16_t palSize;       // The length of the palette
    uint32_t connectCount;  // The number of textures currently using this palette
} gl_palette_data;

// This struct holds hidden globals for videoGL. It is initialized by glInit().
typedef struct gl_hidden_globals
{
    // VRAM blocks management
    // ----------------------

    s_vramBlock *vramBlocksTex; // One for textures
    s_vramBlock *vramBlocksPal; // One for palettes
    int vramLockTex; // Holds the current lock state of the VRAM banks
    int vramLockPal; // Holds the current lock state of the VRAM banks

    // Texture/palette manamenent
    // --------------------------

    // Arrays of texture and palettes. The index to access a texture is the same
    // as the name of that texture. The value of each array component is a
    // pointer to a texture or palette struct. When a texture/palette is
    // generated, the pointer is allocated. When it is freed, the pointer is set
    // deallocated and set to NULL, and the texture name (the array index) is
    // added to the deallocTex or deallocPal array to be reused when required.
    //
    // Note: Getting the activeTexture or activePalette from the arrays will
    // always succeed. glBindTexure() can only set activeTexture and
    // activePalette to an element that exists.
    DynamicArray texturePtrs;
    DynamicArray palettePtrs;

    // Array of names that have been deleted and are ready to be reused. They
    // are just a list of indices to the arrays texturePtrs and palettePtrs that
    // we can reuse.
    DynamicArray deallocTex;
    DynamicArray deallocPal;

    // Number of names available in the list of reusable names
    uint32_t deallocTexSize;
    uint32_t deallocPalSize;

    // Current number of allocated names. It's also the next name that will be
    // used (if there are no reusable names).
    int texCount;
    int palCount;

    // State not related to dynamic memory management
    // ----------------------------------------------

    int activeTexture; // The current active texture name
    int activePalette; // The current active palette name
    u32 clearColor; // Holds the current state of the clear color register
    GL_MATRIX_MODE_ENUM matrixMode; // Holds the current Matrix Mode

    uint8_t isActive; // Has glInit() been called before?
}
gl_hidden_globals;

// This is the actual data of the globals for videoGL.
gl_hidden_globals glGlob;

ARM_CODE void glRotatef32i(int angle, int32_t x, int32_t y, int32_t z)
{
    int32_t axis[3];
    int32_t sin = sinLerp(angle);
    int32_t cos = cosLerp(angle);
    int32_t one_minus_cos = inttof32(1) - cos;

    axis[0] = x;
    axis[1] = y;
    axis[2] = z;

    normalizef32(axis); // Should require passed in normalized?

    MATRIX_MULT3x3 = cos + mulf32(one_minus_cos, mulf32(axis[0], axis[0]));
    MATRIX_MULT3x3 = mulf32(one_minus_cos, mulf32(axis[0], axis[1])) + mulf32(axis[2], sin);
    MATRIX_MULT3x3 = mulf32(mulf32(one_minus_cos, axis[0]), axis[2]) - mulf32(axis[1], sin);

    MATRIX_MULT3x3 = mulf32(mulf32(one_minus_cos, axis[0]), axis[1]) - mulf32(axis[2], sin);
    MATRIX_MULT3x3 = cos + mulf32(mulf32(one_minus_cos, axis[1]), axis[1]);
    MATRIX_MULT3x3 = mulf32(mulf32(one_minus_cos, axis[1]), axis[2]) + mulf32(axis[0], sin);

    MATRIX_MULT3x3 = mulf32(mulf32(one_minus_cos, axis[0]), axis[2]) + mulf32(axis[1], sin);
    MATRIX_MULT3x3 = mulf32(mulf32(one_minus_cos, axis[1]), axis[2]) - mulf32(axis[0], sin);
    MATRIX_MULT3x3 = cos + mulf32(mulf32(one_minus_cos, axis[2]), axis[2]);
}

ARM_CODE void glOrthof32(int left, int right, int bottom, int top,
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

ARM_CODE void gluLookAtf32(int eyex, int eyey, int eyez,
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

    MATRIX_MULT4x3 = -dotf32(eye, side);
    MATRIX_MULT4x3 = -dotf32(eye, up);
    MATRIX_MULT4x3 = -dotf32(eye, forward);
}

ARM_CODE void glFrustumf32(int left, int right, int bottom, int top,
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

ARM_CODE void gluPerspectivef32(int fovy, int aspect, int zNear, int zFar)
{
    int xmin, xmax, ymin, ymax;

    ymax = mulf32(zNear, tanLerp(fovy >> 1));

    ymin = -ymax;
    xmin = mulf32(ymin, aspect);
    xmax = mulf32(ymax, aspect);

    glFrustumf32(xmin, xmax, ymin, ymax, zNear, zFar);
}

ARM_CODE void gluPickMatrix(int x, int y, int width, int height,
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

void glResetMatrixStack(void)
{
    // Make sure there are no push/pops that haven't executed yet
    while (GFX_STATUS & GFX_STATUS_MATRIX_STACK_BUSY)
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

void glMaterialf(GL_MATERIALS_ENUM mode, rgb color)
{
    static uint32_t diffuse_ambient = 0;
    static uint32_t specular_emission = 0;

    switch (mode)
    {
        case GL_AMBIENT:
            diffuse_ambient = (color << 16) | (diffuse_ambient & 0xFFFF);
            break;
        case GL_DIFFUSE:
            diffuse_ambient = color | (diffuse_ambient & 0xFFFF0000);
            break;
        case GL_AMBIENT_AND_DIFFUSE:
            diffuse_ambient = color + (color << 16);
            break;
        case GL_SPECULAR:
            specular_emission = color | (specular_emission & 0xFFFF0000);
            break;
        case GL_SHININESS:
            break;
        case GL_EMISSION:
            specular_emission = (color << 16) | (specular_emission & 0xFFFF);
            break;
    }

    GFX_DIFFUSE_AMBIENT = diffuse_ambient;
    GFX_SPECULAR_EMISSION = specular_emission;
}

ARM_CODE void glTexCoord2f32(int32_t u, int32_t v)
{
    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);
    if (tex)
    {
        int x = (tex->texFormat >> 20) & 7;
        int y = (tex->texFormat >> 23) & 7;
        glTexCoord2t16(f32tot16(mulf32(u, inttof32(8 << x))),
                       f32tot16(mulf32(v, inttof32(8 << y))));
    }
}

//------------------------------------------------------------------------------
// Internal VRAM allocation/deallocation functions. Calling these functions
// outside of videoGL may interfere with normal operations.

int vramBlock_init(s_vramBlock *mb)
{
    // Construct a new block that will be set as the first block, as well as the
    // first empty block.
    struct s_SingleBlock *newBlock = calloc(1, sizeof(struct s_SingleBlock));
    if (newBlock == NULL)
        return 0;

    newBlock->AddrSet = mb->startAddr;
    newBlock->blockSize = (uint32_t)mb->endAddr - (uint32_t)mb->startAddr;

    mb->firstBlock = mb->firstEmpty = newBlock;

    // Default settings and initializations for up to 16 blocks (will increase
    // as limit is reached).
    mb->blockCount = 1;
    mb->deallocCount = 0;

    mb->lastExamined = NULL;
    mb->lastExaminedAddr = NULL;
    mb->lastExaminedSize = 0;

    if (DynamicArrayInit(&mb->blockPtrs, 16) == NULL)
    {
        free(newBlock);
        return 0;
    }
    if (DynamicArrayInit(&mb->deallocBlocks, 16) == NULL)
    {
        DynamicArrayDelete(&mb->blockPtrs);
        free(newBlock);
        return 0;
    }

    for (int i = 0; i < 16; i++)
    {
        // This should always work because we've already allocated 16 elements
        DynamicArraySet(&mb->blockPtrs, i, (void *)0);
        DynamicArraySet(&mb->deallocBlocks, i, (void *)0);
    }

    return 1;
}

s_vramBlock *vramBlock_Construct(uint8_t *start, uint8_t *end)
{
    // Block Container is constructed, with a starting and ending address. Then
    // initialization of the first block is made.
    struct s_vramBlock *mb = malloc(sizeof(s_vramBlock));
    if (mb == NULL)
        return NULL;

    if (start > end)
    {
        mb->startAddr = end;
        mb->endAddr = start;
    }
    else
    {
        mb->startAddr = start;
        mb->endAddr = end;
    }

    if (vramBlock_init(mb) == 0)
    {
        free(mb);
        return NULL;
    }

    return mb;
}

void vramBlock_terminate(s_vramBlock *mb)
{
    // Starts at the container's first block, and goes through each sequential block,
    // deleting them.
    struct s_SingleBlock *curBlock = mb->firstBlock;

    while (curBlock != NULL)
    {
        struct s_SingleBlock *nextBlock = curBlock->node[1];
        free(curBlock);
        curBlock = nextBlock;
    }

    DynamicArrayDelete(&mb->deallocBlocks);
    DynamicArrayDelete(&mb->blockPtrs);
}

void vramBlock_Deconstruct(s_vramBlock *mb)
{
    // Container must exist for deconstructing
    if (mb)
    {
        vramBlock_terminate(mb);
        free(mb);
    }
}

struct s_SingleBlock *vramBlock__allocateBlock(s_vramBlock *mb,
                                               struct s_SingleBlock *block,
                                               uint8_t *addr, uint32_t size)
{
    // Initial tests to ensure allocation is valid
    if (!size || !addr || !block || block->indexOut || addr < block->AddrSet
        || (addr + size) > (block->AddrSet + block->blockSize))
        return NULL;

    // Get pointers to the various blocks, as those may change from allocation
    struct s_SingleBlock **first = &mb->firstBlock;
    struct s_SingleBlock **alloc = &mb->firstAlloc;
    struct s_SingleBlock **empty = &mb->firstEmpty;

    // The nodes in the test block array will change as examinations of pre/post
    // blocks are done
    struct s_SingleBlock *testBlock[4] = {
        block->node[0], block->node[1], block->node[2], block->node[3]
    };

    // Boolean comparisons ( for determining if an empty block set for
    // allocation should be split once, twice, or not at all )
    uint32_t valComp[2] = {
        addr != block->AddrSet,
        addr + size < block->AddrSet + block->blockSize
    };

    for (int i = 0; i < 2; i++)
    {
        // Generate a new block if condition requires it, based on earlier
        // comparison
        if (valComp[i])
        {
            // If comparison check is true, then empty block is split in two
            // empty blocks.  Addresses set, block sizes corrected, and nodes
            // linked between the two. This can be done up to two times,
            // resulting in 3 empty blocks sequentially. The middle in this case
            // will be the true allocated block. If split once in total, either
            // block will be the true block. Also done is examination of the
            // first block and first empty block, which will be set as well.

            struct s_SingleBlock *newBlock = malloc(sizeof(struct s_SingleBlock));
            if (newBlock == NULL)
                return NULL;

            newBlock->indexOut = 0;
            newBlock->AddrSet = block->AddrSet + (i * size);

            if (i)
            {
                newBlock->blockSize = block->blockSize - size;
                block->blockSize = size;
            }
            else
            {
                newBlock->blockSize = (uint32_t)addr - (uint32_t)block->AddrSet;
                block->AddrSet = addr;
                block->blockSize -= newBlock->blockSize;
                if (block == *first)
                    *first = newBlock;
            }

            // To get an idea of what the nodes represent, the first two in the
            // array refer to the immediate pre/post blocks, which can be either
            // empty or allocated blocks. The last two in the array refer to the
            // prior/next blocks of the same type (empty or allocated), which
            // can skip over blocks of a differing type. This allows for quick
            // examination of similar blocks while keeping the organization of
            // all the blocks in an ordered fashion. Think of it like a
            // doubly-doubly linked list.

            newBlock->node[1 - i] = block;
            newBlock->node[i] = testBlock[i];
            newBlock->node[i + 2] = testBlock[i + 2];

            block->node[i] = newBlock;
            if (testBlock[i])
                testBlock[i]->node[1 - i] = newBlock;
            if (testBlock[i + 2])
                testBlock[i + 2]->node[3 - i] = newBlock;

            testBlock[i + 2] = newBlock;

            if (block == *empty)
                *empty = newBlock;
        }
    }

    // Finish off node linking (in this case, NULL is possible, which refers to
    // the end of the block sequences)
    if (testBlock[2])
        testBlock[2]->node[3] = testBlock[3];
    if (testBlock[3])
        testBlock[3]->node[2] = testBlock[2];

    // The condition of examining the first empty block is needed, in case the
    // comparison check early is false for both
    if (block == *empty)
        *empty = block->node[3];

    block->node[2] = testBlock[0];
    block->node[3] = testBlock[1];

    if (testBlock[0])
        testBlock[0]->node[3] = block;
    else
        *alloc = block;

    if (testBlock[1])
        testBlock[1]->node[2] = block;

    return block;
}

uint32_t vramBlock__deallocateBlock(s_vramBlock *mb, struct s_SingleBlock *block)
{
    // Test to see if this is an allocated block
    if (!block->indexOut)
        return 0;

    struct s_SingleBlock **first = &mb->firstBlock;
    struct s_SingleBlock **alloc = &mb->firstAlloc;
    struct s_SingleBlock **empty = &mb->firstEmpty;

    // Unlike empty blocks, allocated blocks can be next to each other to help
    // retain the actual spaces being allocated.
    //
    // This is why when contructing the Test Block for the deallocation process,
    // it fills it with the prior/next links for both sets instead of including
    // the immediate pre/post blocks for figuring out the prior/next closest
    // empty block
    struct s_SingleBlock *testBlock[4] = { block->node[2], block->node[3], block->node[2],
                                           block->node[3] };

    for (int i = 0; i < 2; i++)
    {
        // If the immediate prior/next test link is not the block's immediate
        // prior/next link (meaning an empty block separates them), then set the
        // prior/next link to that empty block.

        if (testBlock[i] != block->node[i])
        {
            testBlock[i + 2] = block->node[i];
        }
        else
        {
            // If not, then scan through the prior/next links until either an
            // empty block is found, or no blocks (NULL) are found
            while (testBlock[i + 2] && testBlock[i + 2]->indexOut)
                testBlock[i + 2] = testBlock[i + 2]->node[i];
        }
    }

    // Begin initial "rewiring" stage for when the block transitions from
    // allocated to empty
    if (testBlock[0])
        testBlock[0]->node[3] = testBlock[1];
    if (testBlock[1])
        testBlock[1]->node[2] = testBlock[0];
    if (testBlock[2])
        testBlock[2]->node[3] = block;
    if (testBlock[3])
        testBlock[3]->node[2] = block;

    block->node[2] = testBlock[2];
    block->node[3] = testBlock[3];
    block->indexOut = 0;

    // If this block was the first allocated block of the group, then pass
    // allocation lead to next one, even if that is NULL
    if (block == *alloc)
        *alloc = testBlock[1];

    for (int i = 0; i < 2; i++)
    {
        if (testBlock[i + 2])
        {
            // If true, then we must do more rewiring, as well as merging blocks
            // together. This also includes reassigning the first block and
            // first empty block if necessary
            if (testBlock[i + 2] == block->node[i])
            {
                block->node[i] = testBlock[i + 2]->node[i];

                if (block->node[i])
                    block->node[i]->node[1 - i] = block;

                block->node[i + 2] = testBlock[i + 2]->node[i + 2];

                if (block->node[i + 2])
                    block->node[i + 2]->node[3 - i] = block;

                block->blockSize += testBlock[i + 2]->blockSize;

                if (!i)
                {
                    block->AddrSet = testBlock[2]->AddrSet;
                    if (testBlock[2] == *first)
                        *first = block;
                }

                if (testBlock[i + 2] == *empty)
                    *empty = block;

                free(testBlock[i + 2]);

                // Even if the above did not happen, there is still a chance the
                // new deallocated block may now be the first empty block, so
                // assign it if that is the case.
            }
            else if (i && testBlock[i + 2] == *empty)
            {
                *empty = block;
            }
        }
    }

    return 1;
}

uint8_t *vramBlock_examineSpecial(s_vramBlock *mb, uint8_t *addr, uint32_t size,
                                  uint8_t align)
{
    // Simple validity tests
    if (!addr || !mb->firstEmpty || !size || align >= 8)
        return NULL;

    // Start with the first empty block
    struct s_SingleBlock *block = mb->firstEmpty;

    // Set these value to 0/NULL (should only be filled in with valid data in
    // case of error), and copy the address to start checking.
    mb->lastExamined = NULL;
    mb->lastExaminedAddr = NULL;
    mb->lastExaminedSize = 0;
    uint8_t *checkAddr = addr;

    // If the address is within a valid block, examine if it will initially fit
    // in it.
    while (block && checkAddr >= block->AddrSet + block->blockSize)
        block = block->node[3];

    if (!block)
        return NULL;

    // Move the address up if before the first valid block
    if (checkAddr < block->AddrSet)
        checkAddr = block->AddrSet;

    uint8_t *bankLock[5] = { 0x0 };
    uint32_t bankSize[5] = { 0x0 };
    uint32_t curBank = 0;

    // Values that hold which banks to examine
    uint32_t isNotMainBank = (checkAddr >= (uint8_t *)VRAM_E ? 1 : 0);
    uint32_t vramCtrl = (isNotMainBank ? VRAM_EFG_CR : VRAM_CR);
    int vramLock = isNotMainBank ? glGlob.vramLockPal : glGlob.vramLockTex;
    uint32_t iEnd = (isNotMainBank ? 3 : 4);

    // Fill in the array with only those banks that are not set for textures or
    // texture palettes
    for (uint32_t i = 0; i < iEnd; i++)
    {
        // if VRAM_ENABLE | ( VRAM_x_TEXTURE | VRAM_x_TEX_PALETTE )
        if (((vramCtrl & 0x83) != 0x83) || (vramLock & 0x1))
        {
            if (isNotMainBank)
            {
                bankLock[curBank] =
                    (i == 0 ? (uint8_t *)VRAM_E : (uint8_t *)VRAM_F + ((i - 1) * 0x4000));
                bankSize[curBank] = (i == 0 ? 0x10000 : 0x4000);
            }
            else
            {
                bankLock[curBank] = (uint8_t *)VRAM_A + (i * 0x20000);
                bankSize[curBank] = 0x20000;
            }
            curBank++;
        }
        vramCtrl >>= 8;
        vramLock >>= 1;
    }
    curBank = 0;

    // Retrieve the available area from this block using the address given
    uint32_t curBlockSize =
        block->blockSize - ((uint32_t)checkAddr - (uint32_t)block->AddrSet);
    do
    {
        // Do address adjustments based on locked banks
        if (bankLock[curBank])
        {
            // Skip to corresponding bank that address is in
            while (bankLock[curBank]
                   && checkAddr >= (bankLock[curBank] + bankSize[curBank]))
            {
                curBank++;
            }

            do
            {
                // Examine is address is within locked bank, and push it to next
                // bank if needed
                if (bankLock[curBank] && checkAddr >= bankLock[curBank]
                    && checkAddr < bankLock[curBank] + bankSize[curBank])
                {
                    checkAddr = bankLock[curBank] + bankSize[curBank];
                }
                else
                {
                    break;
                }
            } while (bankLock[++curBank] != NULL);

            // Continue block and address adjustments
            while (block && checkAddr >= block->AddrSet + block->blockSize)
                block = block->node[3];

            if (!block)
                return NULL;

            if (checkAddr < block->AddrSet)
                checkAddr = block->AddrSet;

            // Adjust the blocks available size based on address location within
            // said block
            if (bankLock[curBank]
                && bankLock[curBank] < block->AddrSet + block->blockSize)
            {
                curBlockSize = (uint32_t)bankLock[curBank] - (uint32_t)checkAddr;
            }
            else
            {
                curBlockSize =
                    block->blockSize - ((uint32_t)checkAddr - (uint32_t)block->AddrSet);
            }
        }

        // Obtained an aligned address, and adjust the available area that can
        // be used
        uint8_t *aligned_checkAddr =
            (uint8_t *)(((uint32_t)checkAddr + ((1 << align) - 1)) & (~((1 << align) - 1)));
        uint32_t excess = ((uint32_t)aligned_checkAddr - (uint32_t)checkAddr);
        curBlockSize -= excess;

        if (curBlockSize >= size)
        {
            mb->lastExamined = block;
            mb->lastExaminedAddr = aligned_checkAddr;
            mb->lastExaminedSize = size;
            return aligned_checkAddr;
        }
        else
        {
            if (bankLock[curBank]
                && bankLock[curBank] < block->AddrSet + block->blockSize)
            {
                checkAddr = bankLock[curBank] + bankSize[curBank];
                curBlockSize = 0;
            }
            else
            {
                block = block->node[3];

                if (!block)
                    return NULL;

                checkAddr = block->AddrSet;
                curBlockSize = block->blockSize;
            }
        }
    } while (block != NULL);

    return NULL;
}

uint32_t vramBlock_allocateSpecial(s_vramBlock *mb, uint8_t *addr, uint32_t size)
{
    // Simple validity tests. Special allocations require "examination" data
    if (!addr || !size || !mb->lastExamined || !mb->lastExaminedAddr)
        return 0;

    if (mb->lastExaminedAddr != addr || mb->lastExaminedSize != size)
        return 0;

    // Can only get here if prior tests passed, meaning a spot is available, and
    // can be allocated
    struct s_SingleBlock *newBlock = vramBlock__allocateBlock(mb, mb->lastExamined, addr, size);
    if (newBlock == NULL)
        return 0;

    // with current implementation, it should never be false if it gets to here
    uint32_t curBlock;

    // Use a prior index if one exists. Else, obtain a new index
    if (mb->deallocCount)
        curBlock = (uint32_t)DynamicArrayGet(&mb->deallocBlocks, mb->deallocCount--);
    else
        curBlock = mb->blockCount++;

    DynamicArraySet(&mb->blockPtrs, curBlock, (void *)newBlock);
    // Clear out examination data
    mb->lastExamined = NULL;
    mb->lastExaminedAddr = NULL;
    mb->lastExaminedSize = 0;
    newBlock->indexOut = curBlock;
    return curBlock;
}

uint32_t vramBlock_allocateBlock(s_vramBlock *mb, uint32_t size, uint8_t align)
{
    // Simple valid tests, such as if there are no more empty blocks as
    // indicated by "firstEmpty"
    if (mb->firstEmpty == NULL || !size || align >= 8)
        return 0;

    // Grab the first empty block, and begin examination for a valid spot from
    // there
    struct s_SingleBlock *block = mb->firstEmpty;
    uint8_t *checkAddr = vramBlock_examineSpecial(mb, block->AddrSet, size, align);
    if (checkAddr == NULL)
        return 0;

    // If execution gets here, then a spot was found, so allocate it
    return vramBlock_allocateSpecial(mb, checkAddr, size);
}

// TODO: The return value of this function isn't checked anywhere, but I'm not
// sure if this is the right approach. All we can do if we fail to deallocate
// memory is crash.
uint32_t vramBlock_deallocateBlock(s_vramBlock *mb, uint32_t index)
{
    // Retrieve the block from the index array, and see if it exists. If it
    // does, and is deallocated (which it should), remove from index list
    struct s_SingleBlock *block = DynamicArrayGet(&mb->blockPtrs, index);

    if (block && vramBlock__deallocateBlock(mb, block))
    {
        // Clear the current element
        DynamicArraySet(&mb->blockPtrs, index, NULL);

        // Add the block to the array of deallocated blocks
        if (!DynamicArraySet(&mb->deallocBlocks, ++mb->deallocCount, (void *)index))
        {
            // It's pretty hard to recover from this. At least, try to detect it
            // in debug builds.
            sassert(false, "Can't add block to deallocBlocks");
            return 0;
        }
        return 1;
    }
    return 0;
}

int vramBlock_deallocateAll(s_vramBlock *mb)
{
    // Reset the entire container
    vramBlock_terminate(mb);
    if (vramBlock_init(mb) == 0)
        return 0;

    return 1;
}

uint8_t *vramBlock_getAddr(s_vramBlock *mb, uint32_t index)
{
    struct s_SingleBlock *getBlock = DynamicArrayGet(&mb->blockPtrs, index);
    if (getBlock)
        return getBlock->AddrSet;

    return NULL;
}

// TODO: This is unused. Remove?
uint32_t vramBlock_getSize(s_vramBlock *mb, uint32_t index)
{
    struct s_SingleBlock *getBlock = DynamicArrayGet(&mb->blockPtrs, index);
    if (getBlock)
        return getBlock->blockSize;

    return 0;
}

//------------------------------------------------------------------------------

static int glWaitForGfxIdle(void)
{
    if (!GFX_BUSY)
        return 0;

    // The geometry engine is busy. Check if it's still busy after 2 VBlanks.
    // TODO: How much time do we need to give it in the worst-case scenario?
    for (int i = 0; i < 2; i++)
    {
        swiWaitForVBlank();
        if (!GFX_BUSY)
            return 0;
    }

    // The geometry engine is still busy. This can happen due to a partial
    // vertex upload by the previous homebrew application (=> ARM7->ARM9 forced
    // reset). So long as the buffer wasn't flushed, this is recoverable, so we
    // attempt to do so.
    for (int i = 0; i < 8; i++)
    {
        GFX_VERTEX16 = 0;
        swiDelay(0x400); // TODO: Do we need such a high arbitrary delay value?
        if (!GFX_BUSY)
            return 0;
    }

    // The geometry engine is still busy. We've given it enough time to fix
    // itself and exhausted all recovery strategies, so at this point all we can
    // do is give up.
    return -1;
}

int glInit(void)
{
    if (glGlob.isActive)
        return 1;

    powerOn(POWER_3D_CORE | POWER_MATRIX); // Enable 3D core & geometry engine

    // Wait for the graphics engine to be idle
    if (glWaitForGfxIdle() != 0)
    {
        powerOff(POWER_3D_CORE | POWER_MATRIX);
        return 0;
    }

    // Allocate the designated layout for each memory block
    glGlob.vramBlocksTex = vramBlock_Construct((uint8_t *)VRAM_A, (uint8_t *)VRAM_E);
    if (glGlob.vramBlocksTex == NULL)
        goto cleanup;
    glGlob.vramBlocksPal = vramBlock_Construct((uint8_t *)VRAM_E, (uint8_t *)VRAM_H);
    if (glGlob.vramBlocksPal == NULL)
        goto cleanup;

    glGlob.vramLockTex = 0;
    glGlob.vramLockPal = 0;

    // init texture globals

    glGlob.clearColor = 0;

    glGlob.activeTexture = 0;
    glGlob.activePalette = 0;
    glGlob.texCount = 1;
    glGlob.palCount = 1;
    glGlob.deallocTexSize = 0;
    glGlob.deallocPalSize = 0;

    // Initialize all of this
    if (DynamicArrayInit(&glGlob.texturePtrs, 16) == NULL)
        goto cleanup;
    if (DynamicArrayInit(&glGlob.palettePtrs, 16) == NULL)
        goto cleanup;
    if (DynamicArrayInit(&glGlob.deallocTex, 16) == NULL)
        goto cleanup;
    if (DynamicArrayInit(&glGlob.deallocPal, 16) == NULL)
        goto cleanup;

    // All of this should succeed because we've just allocated 16 elements as
    // the initial size of each dynamic array. No need to check for errors
    for (int i = 0; i < 16; i++)
    {
        DynamicArraySet(&glGlob.texturePtrs, i, NULL);
        DynamicArraySet(&glGlob.palettePtrs, i, NULL);
        DynamicArraySet(&glGlob.deallocTex, i, NULL);
        DynamicArraySet(&glGlob.deallocPal, i, NULL);
    }

    // Clear the FIFO
    GFX_STATUS |= (1 << 29);

    // Clear overflows from list memory
    glResetMatrixStack();

    // prime the vertex/polygon buffers
    glFlush(0);

    // reset the control bits
    GFX_CONTROL = 0;

    // reset the rear-plane(a.k.a. clear color) to black, ID=0, and opaque
    glClearColor(0, 0, 0, 31);
    glClearPolyID(0);

    // reset the depth to its max
    glClearDepth(GL_MAX_DEPTH);

    GFX_TEX_FORMAT = 0;
    GFX_POLY_FORMAT = 0;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    // Set the GL state as active in the global state struct
    glGlob.isActive = 1;

    return 1;

cleanup:
    DynamicArrayDelete(&glGlob.texturePtrs);
    DynamicArrayDelete(&glGlob.palettePtrs);
    DynamicArrayDelete(&glGlob.deallocTex);
    DynamicArrayDelete(&glGlob.deallocPal);

    vramBlock_Deconstruct(glGlob.vramBlocksTex);
    vramBlock_Deconstruct(glGlob.vramBlocksPal);
    return 0;
}

int glDeinit(void)
{
    if (glGlob.isActive == 0)
        return 1;

    // Wait for the graphics engine to be idle
    if (glWaitForGfxIdle() != 0)
        return 0;

    // Free all texture data (but the arrays remain allocated)
    glResetTextures();

    glGlob.isActive = 0;

    // Deallocate the texture management arrays
    DynamicArrayDelete(&glGlob.texturePtrs);
    DynamicArrayDelete(&glGlob.palettePtrs);
    DynamicArrayDelete(&glGlob.deallocTex);
    DynamicArrayDelete(&glGlob.deallocPal);

    vramBlock_Deconstruct(glGlob.vramBlocksTex);
    vramBlock_Deconstruct(glGlob.vramBlocksPal);

    // Clear the FIFO
    GFX_STATUS |= (1 << 29);

    // reset the control bits
    GFX_CONTROL = 0;

    // prime the vertex/polygon buffers
    glFlush(0);

    powerOff(POWER_3D_CORE | POWER_MATRIX);

    return 1;
}

void glResetTextures(void)
{
    if (glGlob.isActive == 0)
        return;

    glGlob.activeTexture = 0;
    glGlob.activePalette = 0;
    glGlob.texCount = 1;
    glGlob.palCount = 1;
    glGlob.deallocTexSize = 0;
    glGlob.deallocPalSize = 0;

    // Any textures in use will be clean of all their data
    for (unsigned int i = 0; i < glGlob.texturePtrs.cur_size; i++)
    {
        gl_texture_data *texture = DynamicArrayGet(&glGlob.texturePtrs, i);
        if (texture)
        {
            free(texture);
            DynamicArraySet(&glGlob.texturePtrs, i, NULL);
        }
    }

    // Any palettes in use will be cleaned of all their data
    for (unsigned int i = 0; i < glGlob.palettePtrs.cur_size; i++)
    {
        gl_palette_data *palette = DynamicArrayGet(&glGlob.palettePtrs, i);
        if (palette)
        {
            free(palette);
            DynamicArraySet(&glGlob.palettePtrs, i, NULL);
        }
    }

    // Reset all the arrays to 16 elements in case they have grown too much
    DynamicArrayDelete(&glGlob.texturePtrs);
    DynamicArrayDelete(&glGlob.palettePtrs);
    DynamicArrayDelete(&glGlob.deallocTex);
    DynamicArrayDelete(&glGlob.deallocPal);

    // We have just freed a lot of RAM so this should always succeed. The new
    // arrays must be either the same size or bigger than the old ones!
    DynamicArrayInit(&glGlob.texturePtrs, 16);
    DynamicArrayInit(&glGlob.palettePtrs, 16);
    DynamicArrayInit(&glGlob.deallocTex, 16);
    DynamicArrayInit(&glGlob.deallocPal, 16);

    if ((glGlob.texturePtrs.data == NULL) || (glGlob.palettePtrs.data == NULL)
      || (glGlob.deallocTex.data == NULL) || (glGlob.deallocPal.data == NULL))
        sassert(false, "Failed to allocate dynamic arrays");

    for (int i = 0; i < 16; i++)
    {
        DynamicArraySet(&glGlob.texturePtrs, i, NULL);
        DynamicArraySet(&glGlob.palettePtrs, i, NULL);
        DynamicArraySet(&glGlob.deallocTex, i, NULL);
        DynamicArraySet(&glGlob.deallocPal, i, NULL);
    }

    // Clean out both blocks
    if (vramBlock_deallocateAll(glGlob.vramBlocksTex) == 0)
        sassert(false, "Failed to allocate vramBlocksTex");
    if (vramBlock_deallocateAll(glGlob.vramBlocksPal) == 0)
        sassert(false, "Failed to allocate vramBlocksPal");
}

static void removePaletteFromTexture(gl_texture_data *tex)
{
    if (tex == NULL)
        return;

    gl_palette_data *palette = DynamicArrayGet(&glGlob.palettePtrs, tex->palIndex);
    if (palette->connectCount == 0)
        return;

    palette->connectCount--;
    if (palette->connectCount <= 0)
    {
        vramBlock_deallocateBlock(glGlob.vramBlocksPal, palette->palIndex);

        DynamicArraySet(&glGlob.deallocPal, glGlob.deallocPalSize, (void *)tex->palIndex);
        glGlob.deallocPalSize++;

        free(palette);
        DynamicArraySet(&glGlob.palettePtrs, tex->palIndex, NULL);

        // If the active palette is the one we have just removed
        if (glGlob.activePalette == tex->palIndex)
            GFX_PAL_FORMAT = glGlob.activePalette = 0;
    }

    // Clear the palette reference from the texture
    tex->palIndex = 0;
}

// Internal function that returns a new texture name
static int glGenTexture(void)
{
    gl_texture_data *texture = calloc(1, sizeof(gl_texture_data));
    if (texture == NULL)
        return 0;

    if (glGlob.deallocTexSize)
    {
        // If there are texture names in the array of deallocated names,
        // reuse the last one and reduce the size of the array by one.

        int name = (uint32_t)DynamicArrayGet(&glGlob.deallocTex,
                                             glGlob.deallocTexSize - 1);

        if (!DynamicArraySet(&glGlob.texturePtrs, name, texture))
        {
            free(texture);
            return 0;
        }

        glGlob.deallocTexSize--;

        return name;
    }
    else
    {
        // If there is no available texture name to reuse, generate a new name

        int name = glGlob.texCount;

        if (!DynamicArraySet(&glGlob.texturePtrs, name, texture))
        {
            free(texture);
            return 0;
        }

        glGlob.texCount++;

        return name;
    }
}

// Create integer names for your table. Takes n as the number of textures to
// generate and a pointer to the names array that it needs to fill. Returns 1 if
// succesful and 0 if out of texture names.
int glGenTextures(int n, int *names)
{
    // Don't do anything if can't add all generated textures
    if ((glGlob.texCount - glGlob.deallocTexSize + n) >= MAX_TEXTURES)
        return 0;

    // Generate texture names for each element. If any of the names can't be
    // generated, delete all the ones we have allocated up to this point and
    // return failure.
    for (int index = 0; index < n; index++)
    {
        int name = glGenTexture();
        if (name == 0)
        {
            // The current index is equal to the number of names we've
            // generated, which is the number of names we need to delete.
            glDeleteTextures(index, names);
            return 0;
        }

        names[index] = name;
    }

    return 1;
}

// Delete integer names from your table. Takes n as the number of textures to
// delete and a pointer to the names array that it needs to remove. Returns 1 if
// successful and 0 if out of texture names.
int glDeleteTextures(int n, int *names)
{
    for (int index = 0; index < n; index++)
    {
        if (names[index] == 0)
            continue;

        // The developer may have passed invalid values to this function.
        sassert(names[index] <= MAX_TEXTURES, "Invalid texture name");

        // Save this texture name in the deallocated texture name array so that we can
        // reuse it later.
        if (!DynamicArraySet(&glGlob.deallocTex, glGlob.deallocTexSize,
                             (void *)names[index]))
        {
            // This is quite unexpected, and I imagine that most people won't
            // check the error code of glDeleteTextures(), so it's better to add
            // an assert here in case the bug can be found in debug builds.
            sassert(false, "Can't add name to deallocTex array");
            return 0;
        }

        glGlob.deallocTexSize++;

        // If this name had an assigned texture to it, delete it
        gl_texture_data *texture = DynamicArrayGet(&glGlob.texturePtrs, names[index]);
        if (texture)
        {
            // Clear out the texture blocks
            if (texture->texIndex)
            {
                // Delete extra texture for GL_COMPRESSED, if it exists
                if (texture->texIndexExt)
                    vramBlock_deallocateBlock(glGlob.vramBlocksTex, texture->texIndexExt);

                vramBlock_deallocateBlock(glGlob.vramBlocksTex, texture->texIndex);
            }

            // Clear out the palette if this texture name is the last
            // texture using it
            if (texture->palIndex)
                removePaletteFromTexture(texture);

            free(texture);

            // Clear pointer to mark the name as not having a texture
            DynamicArraySet(&glGlob.texturePtrs, names[index], NULL);
        }

        // Zero out register if the active texture was being deleted
        if (glGlob.activeTexture == names[index])
        {
            GFX_TEX_FORMAT = 0;
            glGlob.activeTexture = 0;
        }

        // Finally, clear the component from the array passed by the user
        names[index] = 0;
    }

    return 1;
}

static uint16_t *vramGetBank(uint16_t *addr)
{
    const uint16_t *vram_i_end = VRAM_I + ((16 * 1024) / sizeof(u16));

    if (addr >= VRAM_A && addr < VRAM_B)
        return VRAM_A;
    else if (addr >= VRAM_B && addr < VRAM_C)
        return VRAM_B;
    else if (addr >= VRAM_C && addr < VRAM_D)
        return VRAM_C;
    else if (addr >= VRAM_D && addr < VRAM_E)
        return VRAM_D;
    else if (addr >= VRAM_E && addr < VRAM_F)
        return VRAM_E;
    else if (addr >= VRAM_F && addr < VRAM_G)
        return VRAM_F;
    else if (addr >= VRAM_G && addr < VRAM_H)
        return VRAM_G;
    else if (addr >= VRAM_H && addr < VRAM_I)
        return VRAM_H;
    else if (addr >= VRAM_I && addr < vram_i_end)
        return VRAM_I;

    sassert(0, "Address not in VRAM");
    return NULL;
}

// Lock a designated vram bank to prevent consideration of the bank when
// allocating. This is not an actual OpenGL function.
int glLockVRAMBank(uint16_t *addr)
{
    uint16_t *actualVRAMBank = vramGetBank(addr);
    if (actualVRAMBank < VRAM_A || actualVRAMBank > VRAM_G)
        return 0;

    // Texture banks
    if (actualVRAMBank == VRAM_A)
        glGlob.vramLockTex |= BIT(0);
    else if (actualVRAMBank == VRAM_B)
        glGlob.vramLockTex |= BIT(1);
    else if (actualVRAMBank == VRAM_C)
        glGlob.vramLockTex |= BIT(2);
    else if (actualVRAMBank == VRAM_D)
        glGlob.vramLockTex |= BIT(3);
    // Palette banks
    else if (actualVRAMBank == VRAM_E)
        glGlob.vramLockPal |= BIT(0);
    else if (actualVRAMBank == VRAM_F)
        glGlob.vramLockPal |= BIT(1);
    else if (actualVRAMBank == VRAM_G)
        glGlob.vramLockPal |= BIT(2);

    return 1;
}

// Unlock a designated vram bank to allow consideration of the bank when
// allocating. This is not an actual OpenGL function
int glUnlockVRAMBank(uint16_t *addr)
{
    uint16_t *actualVRAMBank = vramGetBank(addr);
    if (actualVRAMBank < VRAM_A || actualVRAMBank > VRAM_G)
        return 0;

    // Texture banks
    if (actualVRAMBank == VRAM_A)
        glGlob.vramLockTex &= ~BIT(0);
    else if (actualVRAMBank == VRAM_B)
        glGlob.vramLockTex &= ~BIT(1);
    else if (actualVRAMBank == VRAM_C)
        glGlob.vramLockTex &= ~BIT(2);
    else if (actualVRAMBank == VRAM_D)
        glGlob.vramLockTex &= ~BIT(3);
    // Palette banks
    else if (actualVRAMBank == VRAM_E)
        glGlob.vramLockPal &= ~BIT(0);
    else if (actualVRAMBank == VRAM_F)
        glGlob.vramLockPal &= ~BIT(1);
    else if (actualVRAMBank == VRAM_G)
        glGlob.vramLockPal &= ~BIT(2);

    return 1;
}

// Set the current named texture to the active texture. The target is ignored as
// all DS textures are 2D.
int glBindTexture(int target, int name)
{
    (void)target;

    // No reason to process if name is the active texture
    if (glGlob.activeTexture == name)
        return 0;

    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, name);

    // Has the name been generated with glGenTextures()?
    if (tex == NULL)
    {
        GFX_TEX_FORMAT = 0;
        GFX_PAL_FORMAT = 0;
        glGlob.activePalette = 0;
        glGlob.activeTexture = 0;
        return 0;
    }

    GFX_TEX_FORMAT = tex->texFormat;
    glGlob.activeTexture = name;

    // Set palette if exists
    if (tex->palIndex)
    {
        gl_palette_data *pal = DynamicArrayGet(&glGlob.palettePtrs, tex->palIndex);
        sassert(pal, "tex->palIndex is set, but no pal available");
        GFX_PAL_FORMAT = pal->addr;
        glGlob.activePalette = tex->palIndex;
    }
    else
    {
        GFX_PAL_FORMAT = glGlob.activePalette = 0;
    }

    return 1;
}

// Load a 15-bit color format palette into palette memory, and set it to the
// currently bound texture.
int glColorTableEXT(int target, int empty1, uint16_t width, int empty2, int empty3,
                    const void *table)
{
    (void)target;
    (void)empty1;
    (void)empty2;
    (void)empty3;

    // We can only load a palette if there is an active texture
    if (!glGlob.activeTexture)
        return 0;

    gl_texture_data *texture = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);

    if (texture->palIndex) // Remove prior palette if exists
        removePaletteFromTexture(texture);

    // Exit if color count is 0 (helpful in emptying the palette for the active texture).
    // This isn't considered an error.
    if (width == 0)
        return 1;

    // Allocate new palette block based on the texture's format
    uint32_t colFormat = (texture->texFormat >> 26) & 0x7;

    uint32_t colFormatVal =
        ((colFormat == GL_RGB4 || (colFormat == GL_NOTEXTURE && width <= 4)) ? 3 : 4);
    uint8_t *checkAddr = vramBlock_examineSpecial(glGlob.vramBlocksPal,
        (uint8_t *)VRAM_E, width << 1, colFormatVal);

    if (checkAddr == NULL)
    {
        // Failed to find enough space for the palette
        sassert(texture->palIndex == 0, "glColorTableEXT didn't clear palette");
        GFX_PAL_FORMAT = glGlob.activePalette = 0;
        return 0;
    }

    // Calculate the address, logical and actual, of where the palette will go
    uint16_t *baseBank = vramGetBank((uint16_t *)checkAddr);
    uint32_t addr = ((uint32_t)checkAddr - (uint32_t)baseBank);
    uint8_t offset = 0;

    if (baseBank == VRAM_F)
        offset = (VRAM_F_CR >> 3) & 3;
    else if (baseBank == VRAM_G)
        offset = (VRAM_G_CR >> 3) & 3;
    addr += ((offset & 0x1) * 0x4000) + ((offset & 0x2) * 0x8000);

    addr >>= colFormatVal;
    if (colFormatVal == 3 && addr >= 0x2000)
    {
        // Palette location not good because 4 color mode cannot extend
        // past 64K texture palette space
        GFX_PAL_FORMAT = glGlob.activePalette = 0;
        return 0;
    }

    gl_palette_data *palette = malloc(sizeof(gl_palette_data));
    if (palette == NULL)
        return 0;

    // Get a new palette name (either reused or new)
    if (glGlob.deallocPalSize)
    {
        uint32_t palIndex = (uint32_t)DynamicArrayGet(&glGlob.deallocPal,
                                                      glGlob.deallocPalSize - 1);

        if (!DynamicArraySet(&glGlob.palettePtrs, palIndex, palette))
        {
            free(palette);
            return 0;
        }

        texture->palIndex = palIndex;
        glGlob.deallocPalSize--;
    }
    else
    {
        uint32_t palIndex = glGlob.palCount;

        if (!DynamicArraySet(&glGlob.palettePtrs, palIndex, palette))
        {
            free(palette);
            return 0;
        }

        texture->palIndex = palIndex;
        glGlob.palCount++;
    }

    // Lock the free space we have found
    palette->palIndex = vramBlock_allocateSpecial(glGlob.vramBlocksPal, checkAddr, width << 1);
    sassert(palette->palIndex != 0, "Failed to lock free palette VRAM");

    palette->vramAddr = checkAddr;
    palette->addr = addr;

    palette->connectCount = 1;
    palette->palSize = width << 1;

    GFX_PAL_FORMAT = palette->addr;
    glGlob.activePalette = texture->palIndex;

    // Exit if table is NULL (helpful to allocate VRAM without filling).
    // This isn't considered an error.
    if (table == NULL)
        return 1;

    // Copy straight to VRAM, and assign a palette name
    uint32_t tempVRAM = VRAM_EFG_CR;
    uint16_t *startBank = vramGetBank((uint16_t *)palette->vramAddr);
    uint16_t *endBank = vramGetBank((uint16_t *)((char *)palette->vramAddr + (width << 1) - 1));

    // Only set to LCD mode the banks that we need to modify, not all of them.
    // Some of them may be used for purposes other than texture palettes.
    do
    {
        if (startBank == VRAM_E)
        {
            vramSetBankE(VRAM_E_LCD);
            startBank += 0x8000;
        }
        else if (startBank == VRAM_F)
        {
            vramSetBankF(VRAM_F_LCD);
            startBank += 0x2000;
        }
        else if (startBank == VRAM_G)
        {
            vramSetBankG(VRAM_G_LCD);
            startBank += 0x2000;
        }
    }
    while (startBank <= endBank);

    memcpy(palette->vramAddr, table, width * 2);
    vramRestoreBanks_EFG(tempVRAM);

    return 1;
}

// Load a 15-bit color format palette into a specific spot in a currently bound
// texture's existing palette.
int glColorSubTableEXT(int target, int start, int count, int empty1, int empty2,
                       const void *data)
{
    (void)target;
    (void)empty1;
    (void)empty2;

    if (count <= 0)
        return 0;

    if (!glGlob.activePalette)
        return 0;

    gl_palette_data *palette = DynamicArrayGet(&glGlob.palettePtrs, glGlob.activePalette);

    if (start >= 0 && (start + count) <= (palette->palSize >> 1))
    {
        uint32_t tempVRAM = vramSetBanks_EFG(VRAM_E_LCD, VRAM_F_LCD, VRAM_G_LCD);
        memcpy((char *)palette->vramAddr + (start * 2), data, count * 2);
        vramRestoreBanks_EFG(tempVRAM);

        return 1;
    }

    return 0;
}

// Retrieve a 15-bit color format palette from the palette memory of the
// currently bound texture.
int glGetColorTableEXT(int target, int empty1, int empty2, void *table)
{
    (void)target;
    (void)empty1;
    (void)empty2;

    if (!glGlob.activePalette)
        return 0;

    gl_palette_data *palette = DynamicArrayGet(&glGlob.palettePtrs, glGlob.activePalette);

    uint32_t tempVRAM = vramSetBanks_EFG(VRAM_E_LCD, VRAM_F_LCD, VRAM_G_LCD);
    memcpy(table, palette->vramAddr, palette->palSize);
    vramRestoreBanks_EFG(tempVRAM);

    return 1;
}

// Set the active texture with a palette set with another texture. This is not
// an actual OpenGL function.
int glAssignColorTable(int target, int name)
{
    (void)target;

    if (!glGlob.activeTexture)
        return 0;

    // Only allow assigning from a texture different from the active one
    if (glGlob.activeTexture == name)
        return 0;

    gl_texture_data *texture = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);
    gl_texture_data *texCopy = DynamicArrayGet(&glGlob.texturePtrs, name);

    // Remove prior palette from active texture if it exists
    if (texture->palIndex)
        removePaletteFromTexture(texture);

    if (texCopy && texCopy->palIndex)
    {
        texture->palIndex = texCopy->palIndex;

        gl_palette_data *palette = DynamicArrayGet(&glGlob.palettePtrs, texture->palIndex);

        palette->connectCount++;
        GFX_PAL_FORMAT = palette->addr;
        glGlob.activePalette = texture->palIndex;

        return 1;
    }
    else
    {
        GFX_PAL_FORMAT = glGlob.activePalette = texture->palIndex = 0;

        return 0;
    }
}

// Although named the same as its OpenGL counterpart it is not compatible.
// Effort may be made in the future to make it so.
int glTexParameter(int target, int param)
{
    (void)target;

    if (glGlob.activeTexture == 0)
    {
        GFX_TEX_FORMAT = 0;
        return 0;
    }

    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);
    GFX_TEX_FORMAT = tex->texFormat = (tex->texFormat & 0x1FF0FFFF) | param;
    return 1;
}

// Gets a pointer to the VRAM address that contains the texture.
void *glGetTexturePointer(int name)
{
    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, name);

    if (tex)
        return tex->vramAddr;
    else
        return NULL;
}

// Gets a pointer to the VRAM address that contains the extra data of the
// compressed texture.
void *glGetTextureExtPointer(int name)
{
    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, name);
    if (tex == NULL)
        return NULL;

    uint32_t format = (tex->texFormat >> 26) & 0x07;
    if (format != GL_COMPRESSED)
        return NULL;

    return vramBlock_getAddr(glGlob.vramBlocksTex, tex->texIndexExt);
}

// Gets a pointer to the VRAM address that contains the palette.
void *glGetColorTablePointer(int name)
{
    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, name);
    if (tex == NULL)
        return NULL;

    if (tex->palIndex == 0)
        return NULL;

    gl_palette_data *pal = DynamicArrayGet(&glGlob.palettePtrs, tex->palIndex);
    if (pal == NULL)
        return NULL;

    return pal->vramAddr;
}

// Retrieves the currently bound texture's format.
u32 glGetTexParameter(void)
{
    if (!glGlob.activeTexture)
        return 0;

    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);
    return tex->texFormat;
}

// Retrieves information pertaining to the currently bound texture's palette.
int glGetColorTableParameterEXT(int target, int pname, int *params)
{
    (void)target;

    if (!glGlob.activePalette)
    {
        *params = -1;
        return 0;
    }

    gl_palette_data *pal = DynamicArrayGet(&glGlob.palettePtrs, glGlob.activePalette);

    if (pname == GL_COLOR_TABLE_FORMAT_EXT)
        *params = pal->addr;
    else if (pname == GL_COLOR_TABLE_WIDTH_EXT)
        *params = pal->palSize >> 1;
    else
        *params = -1;

    return 1;
}

// Similer to glTextImage2D from gl it takes a pointer to data. Empty fields and
// target are unused but provided for code compatibility. Type is simply the
// texture type (GL_RGB, GL_RGB8 ect...)
int glTexImage2D(int target, int empty1, GL_TEXTURE_TYPE_ENUM type, int sizeX, int sizeY,
                 int empty2, int param, const void *texture)
{
    (void)empty1;
    (void)empty2;

    uint32_t size = 0;
    // Represents the number of bits per pixels for each format
    uint32_t typeSizes[9] =
    {
        0, 8, 2, 4, 8, 3, 8, 16, 16
    };

    // There must be an active texture for this function to work
    if (!glGlob.activeTexture)
        return 0;

    // Check if the texture format is invalid
    if (type > GL_RGB)
        return 0;

    // Values between 0 and 7 (inclusive) represent internal texture sizes as
    // represented in GPU registers. Powers of 2 between 8 and 1024 (inclusive)
    // represent actual sizes in pixels, which need to be converted to the
    // internal hardware representations.
    if (sizeX >= 8)
        sizeX = glTexSizeToEnum(sizeX);
    if (sizeY >= 8)
        sizeY = glTexSizeToEnum(sizeY);

    // Either the value provided by the user was negative to begin with, or the
    // conversion of glTexSizeToEnum() failed because the size isn't a valid
    // power of two.
    if ((sizeX < 0) || (sizeY < 0))
        return 0;

    size = 1 << (sizeX + sizeY + 6);

    switch (type)
    {
        case GL_RGB:
        case GL_RGBA:
            size = size << 1;
            break;
        case GL_RGB4:
        case GL_COMPRESSED:
            size = size >> 2;
            break;
        case GL_RGB16:
            size = size >> 1;
            break;
        default:
            break;
    }
    if (!size)
        return 0;

    if (type == GL_NOTEXTURE)
        size = 0;

    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);

    // If there is a texture already and its size and bits per pixel are the
    // same as the ones of the new texture, reuse the old buffer. If not, clear
    // the texture data so that a new buffer is allocated.
    uint32_t texType = ((tex->texFormat >> 26) & 0x07);
    if ((tex->texSize != size) || (typeSizes[texType] != typeSizes[type]))
    {
        if (tex->texIndexExt)
            vramBlock_deallocateBlock(glGlob.vramBlocksTex, tex->texIndexExt);

        if (tex->texIndex)
            vramBlock_deallocateBlock(glGlob.vramBlocksTex, tex->texIndex);

        tex->texIndex = tex->texIndexExt = 0;
        tex->vramAddr = NULL;
    }

    tex->texSize = size;

    // Allocate a new space for the texture in VRAM
    if (!tex->texIndex)
    {
        if (type == GL_NOTEXTURE)
        {
            // Don't allocate a new texture, only deallocate the old one.
            tex->vramAddr = NULL;
            tex->texFormat = 0;
            return 1;
        }
        else if (type != GL_COMPRESSED)
        {
            tex->texIndex = vramBlock_allocateBlock(glGlob.vramBlocksTex, tex->texSize, 3);
            // This may fail, but it is handled below.
        }
        else // if (type == GL_COMPRESSED)
        {
            uint8_t *vramBAddr = (uint8_t *)VRAM_B;
            uint8_t *vramACAddr = NULL;
            uint8_t *vramBFound, *vramACFound;
            uint32_t vramBAllocSize = size >> 1;

            // The main texture chunk needs to fit in one VRAM bank (A or C)
            if (size > 128 * 1024)
                return 0;

            // In theory, any VRAM bank mapped as texture slot 1 can work,
            // but let's restrict ourselves to using VRAM_B
            if (VRAM_B_CR != (VRAM_ENABLE | VRAM_B_TEXTURE_SLOT1))
                return 0;

            if ((VRAM_A_CR != (VRAM_ENABLE | VRAM_A_TEXTURE_SLOT0)) &&
                (VRAM_C_CR != (VRAM_ENABLE | VRAM_C_TEXTURE_SLOT2)))
                return 0;

            // The process of finding a valid spot for compressed textures
            // is as follows:
            //
            // - Examine first available spot in VRAM_B for the header data.
            // - Extrapolate where the tile data would go in VRAM_A or VRAM_C if
            //   the spot in VRAM_B were used.
            // - Check the extrapolated area to see if it is an empty spot.
            // - If not, then adjust the header spot in VRAM_B by a ratio amount
            //   found by the tile spot.

            while (1)
            {
                // Check designated opening, and return available spot
                vramBFound = vramBlock_examineSpecial(glGlob.vramBlocksTex,
                                                      vramBAddr, vramBAllocSize, 2);

                // Make sure that the space found in VRAM_B is completely in
                // it, and not extending out of it. If it extends out of it,
                // there is no space in VRAM_B, so the texture can't be loaded.
                if ((vramGetBank((uint16_t *)vramBFound) != VRAM_B) ||
                    (vramGetBank((uint16_t *)(vramBFound + vramBAllocSize) - 1) != VRAM_B))
                {
                    return 0;
                }

                uint32_t offset = (uint32_t)vramBFound - (uint32_t)VRAM_B;

                // Make sure it is completely on either half of VRAM_B. The
                // first half maps to VRAM_A, the second half maps to VRAM_C.
                if ((offset < 0x10000) && (offset + vramBAllocSize > 0x10000))
                {
                    // If the slot is in both halves, try again starting from
                    // the second half of VRAM_B.
                    vramBAddr = (uint8_t *)VRAM_B + 0x10000;
                    continue;
                }

                // Retrieve the tile location in VRAM_A or VRAM_C
                vramACAddr = (uint8_t *)(offset >= 0x10000 ? VRAM_C : VRAM_A)
                             + ((offset & 0xFFFF) << 1);

                vramACFound = vramBlock_examineSpecial(glGlob.vramBlocksTex,
                                                       vramACAddr, size, 3);
                if (vramACAddr == vramACFound)
                {
                    // Valid addresses found, lock them for this texture.
                    tex->texIndex = vramBlock_allocateSpecial(glGlob.vramBlocksTex,
                                                              vramACFound, size);
                    tex->texIndexExt = vramBlock_allocateSpecial(
                        glGlob.vramBlocksTex,
                        vramBlock_examineSpecial(glGlob.vramBlocksTex, vramBFound,
                                                 vramBAllocSize, 2),
                        vramBAllocSize);

                    // This should never happen because we have just checked
                    // that they are free.
                    sassert((tex->texIndex != 0) && (tex->texIndexExt != 0),
                            "Failed to lock tex and texExt VRAM");
                    break;
                }

                // If we started to look for space from VRAM_A, but VRAM_A is
                // completely full (or not mapped for textures) it is possible
                // that vramACFound is inside VRAM_B. If that happens, restart
                // from VRAM_C.
                if (vramGetBank((uint16_t *)vramACFound) == VRAM_B)
                {
                    vramBAddr = (uint8_t *)VRAM_B + 0x10000;
                    continue;
                }

                // Advance the address in VRAM_B by the difference found with
                // VRAM_A/VRAM_C, divided by 2. Then, try again there.
                vramBAddr += ((uint32_t)vramACFound - (uint32_t)vramACAddr) >> 1;
            }
        }

        if (tex->texIndex)
        {
            tex->vramAddr = vramBlock_getAddr(glGlob.vramBlocksTex, tex->texIndex);
            tex->texFormat = (sizeX << 20) | (sizeY << 23)
                             | ((type == GL_RGB ? GL_RGBA : type) << 26)
                             | (((uint32_t)tex->vramAddr >> 3) & 0xFFFF);
        }
        else
        {
            tex->vramAddr = NULL;
            tex->texFormat = 0;
            return 0;
        }
    }
    else
    {
        // This point is reached if there already was a buffer that we can use
        // for the new texture. We only need to update the texture information.
        tex->texFormat = (sizeX << 20) | (sizeY << 23)
                         | ((type == GL_RGB ? GL_RGBA : type) << 26)
                         | (tex->texFormat & 0xFFFF);
    }

    glTexParameter(target, param);

    // If a texture has been provided, copy the texture data into VRAM.
    if ((type != GL_NOTEXTURE) && (texture != NULL))
    {
        uint32_t vramTemp = VRAM_CR;
        uint16_t *startBank = vramGetBank((uint16_t *)tex->vramAddr);
        uint16_t *endBank = vramGetBank((uint16_t *)((char *)tex->vramAddr + size - 1));

        // Only set to LCD mode the banks that we need to modify, not all of
        // them. Some of them may be used for purposes other than textures.
        do
        {
            if (startBank == VRAM_A)
                vramSetBankA(VRAM_A_LCD);
            else if (startBank == VRAM_B)
                vramSetBankB(VRAM_B_LCD);
            else if (startBank == VRAM_C)
                vramSetBankC(VRAM_C_LCD);
            else if (startBank == VRAM_D)
                vramSetBankD(VRAM_D_LCD);
            startBank += 0x10000;
        }
        while (startBank <= endBank);

        if (type == GL_RGB)
        {
            const uint32_t *src = texture;
            uint32_t *dest = (uint32_t *)tex->vramAddr;
            size >>= 2;
            while (size--)
            {
                *dest++ = *src | 0x80008000;
                src++;
            }
        }
        else
        {
            // Use the CPU to copy data so that this process can be interrupted
            // by hardware interrupts. The minumum texture size is 8x8 pixels,
            // which is 16 bytes in total for a GL_RGB4 or GL_COMPRESSED
            // texture. This is a multiple of a word.
            memcpy(tex->vramAddr, texture, size);

            if (type == GL_COMPRESSED)
            {
                // Extra texture data is always placed in VRAM bank B
                vramSetBankB(VRAM_B_LCD);

                // The size of the ext data is half the size of the regular
                // texture data. The minimum size is 16/2, which is a multiple
                // of a word.
                memcpy(vramBlock_getAddr(glGlob.vramBlocksTex, tex->texIndexExt),
                       (const char *)texture + tex->texSize, size / 2);
            }
        }
        vramRestorePrimaryBanks(vramTemp);
    }

    return 1;
}

void glGetFixed(const GL_GET_ENUM param, int *f)
{
    switch (param)
    {
        case GL_GET_MATRIX_VECTOR:
            while (GFX_BUSY);
            for (int i = 0; i < 9; i++)
                f[i] = MATRIX_READ_VECTOR[i];
            break;

        case GL_GET_MATRIX_CLIP:
            while (GFX_BUSY);
            for (int i = 0; i < 16; i++)
                f[i] = MATRIX_READ_CLIP[i];
            break;

        case GL_GET_MATRIX_PROJECTION:
            glMatrixMode(GL_POSITION);
            // Save the current state of the position matrix
            glPushMatrix();
            // Load an identity matrix into the position matrix so that the clip
            // matrix = projection matrix
            glLoadIdentity();
            // Wait until the graphics engine has stopped to read matrices
            while (GFX_BUSY);
            // Read out the projection matrix
            for (int i = 0; i < 16; i++)
                f[i] = MATRIX_READ_CLIP[i];
            // Restore the position matrix
            glPopMatrix(1);
            break;

        case GL_GET_MATRIX_POSITION:
            glMatrixMode(GL_PROJECTION);
            // Save the current state of the projection matrix
            glPushMatrix();
            // Load an identity matrix into the projection matrix so that the
            // clip matrix = position matrix
            glLoadIdentity();
            // Wait until the graphics engine has stopped to read matrices
            while (GFX_BUSY);
            // Read out the position matrix
            for (int i = 0; i < 16; i++)
                f[i] = MATRIX_READ_CLIP[i];
            // Restore the projection matrix
            glPopMatrix(1);
            break;

        default:
            break;
    }
}

void glGetInt(GL_GET_ENUM param, int *i)
{
    gl_texture_data *tex;

    switch (param)
    {
        case GL_GET_POLYGON_RAM_COUNT:
            while (GFX_BUSY);
            *i = GFX_POLYGON_RAM_USAGE;
            break;

        case GL_GET_VERTEX_RAM_COUNT:
            while (GFX_BUSY);
            *i = GFX_VERTEX_RAM_USAGE;
            break;

        case GL_GET_TEXTURE_WIDTH:
            tex = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);
            if (tex)
                *i = 8 << ((tex->texFormat >> 20) & 7);
            break;

        case GL_GET_TEXTURE_HEIGHT:
            tex = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);
            if (tex)
                *i = 8 << ((tex->texFormat >> 23) & 7);
            break;

        default:
            break;
    }
}

void glTexCoord2f(float s, float t)
{
    gl_texture_data *tex = DynamicArrayGet(&glGlob.texturePtrs, glGlob.activeTexture);
    if (tex)
    {
        int x = (tex->texFormat >> 20) & 7;
        int y = (tex->texFormat >> 23) & 7;
        glTexCoord2t16(floattot16(s * (8 << x)), floattot16(t * (8 << y)));
    }
}

void glCallList(const void *list)
{
    sassert(list != NULL, "glCallList received a null display list pointer");

    const u32 *ptr = list;
    u32 count = *ptr++;

    sassert(count != 0, "glCallList received a display list of size 0");

    // Flush the area that we are going to DMA
    DC_FlushRange(ptr, count * 4);

    // There is a hardware bug that affects DMA when there are multiple channels
    // active, under certain conditions. Instead of checking for said
    // conditions, simply ensure that there are no DMA channels active.
    while (dmaBusy(0) || dmaBusy(1) || dmaBusy(2) || dmaBusy(3));

    // Send the packed list asynchronously via DMA to the FIFO
    dmaSetParams(0, ptr, (void*) &GFX_FIFO, DMA_FIFO | count);
    while (dmaBusy(0));
}

void glClearColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
{
    glGlob.clearColor = (glGlob.clearColor & 0xFFE08000)
                         | (0x7FFF & RGB15(red, green, blue))
                         | ((alpha & 0x1F) << 16);
    GFX_CLEAR_COLOR = glGlob.clearColor;
}

void glClearPolyID(uint8_t ID)
{
    glGlob.clearColor = (glGlob.clearColor & 0xC0FFFFFF) | ((ID & 0x3F) << 24);
    GFX_CLEAR_COLOR = glGlob.clearColor;
}

void glClearFogEnable(bool enable)
{
    glGlob.clearColor = (glGlob.clearColor & 0xFFFF7FFF) | (enable ? BIT(15) : 0);
    GFX_CLEAR_COLOR = glGlob.clearColor;
}
