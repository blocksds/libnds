// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Dave Murphy (WinterMute)

#include <nds/arm9/exceptions.h>
#include <nds/ndstypes.h>

void setExceptionHandler(VoidFn handler)
{
    EXCEPTION_VECTOR = enterException;
    exceptionC = handler;
}
