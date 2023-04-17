// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/arm9/exceptions.h>
#include <nds/ndstypes.h>

void setExceptionHandler(VoidFn handler)
{
    EXCEPTION_VECTOR = enterException;
    exceptionC = handler;
}
