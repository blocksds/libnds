// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2011 Dave Murphy (WinterMute)
// Copyright (C) 2017 fincs

#include <nds/bios.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

int swiDecompressLZSSVram(const void *source, void *destination,
                          uint32_t toGetSize, TDecompressionStream *stream)
{
    if (isDSiMode())
        return swiDecompressLZSSVramTWL(source,destination,toGetSize,stream);
    else
        return swiDecompressLZSSVramNTR(source,destination,toGetSize,stream);
}
