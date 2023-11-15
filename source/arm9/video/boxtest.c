// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Michael Noland (joat)
// Copyright (C) 2005 Jason Rogers (dovoto)
// Copyright (C) 2005 Dave Murphy (WinterMute)

// Code for performing hardware box test against viewing frustrum

#include <nds/arm9/boxtest.h>
#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>

void BoxTest_Asynch(v16 x, v16 y, v16 z, v16 width, v16 height, v16 depth)
{
    glPolyFmt(BIT(12) | BIT(13));
    glBegin(GL_TRIANGLES);
    glEnd();

    GFX_BOX_TEST = VERTEX_PACK(x, y);
    GFX_BOX_TEST = VERTEX_PACK(z, width);
    GFX_BOX_TEST = VERTEX_PACK(height, depth);
}

int BoxTest(v16 x, v16 y, v16 z, v16 width, v16 height, v16 depth)
{
    glPolyFmt(BIT(12) | BIT(13));
    glBegin(GL_TRIANGLES);
    glEnd();

    GFX_BOX_TEST = VERTEX_PACK(x, y);
    GFX_BOX_TEST = VERTEX_PACK(z, width);
    GFX_BOX_TEST = VERTEX_PACK(height, depth);

    while (GFX_STATUS & GFX_STATUS_TEST_BUSY);

    return GFX_STATUS & GFX_STATUS_TEST_INSIDE;
}
