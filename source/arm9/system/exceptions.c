// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/ndstypes.h>
#include <nds/arm9/exceptions.h>

void setExceptionHandler(VoidFn handler)
{
    EXCEPTION_VECTOR = enterException;
    *exceptionC = handler;
}

