// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka

// Camera control for the ARM9

#include <stdio.h>

#include <nds.h>
#include <nds/fifomessages.h>

// High-level functions

u8 cameraActiveDevice = CAMERA_NONE;

u8 cameraGetActive(void)
{
    return cameraActiveDevice;
}

extern bool cameraInitTWL(void);
bool cameraInit(void)
{
    if (!isDSiMode())
        return false;
    return cameraInitTWL();
}

extern bool cameraDeinitTWL(void);
bool cameraDeinit(void)
{
    if (!isDSiMode())
        return false;
    return cameraDeinitTWL();
}

extern bool cameraSelectTWL(CameraDevice device);
bool cameraSelect(CameraDevice device)
{
    if (!isDSiMode())
        return false;
    return cameraSelectTWL(device);
}

extern bool cameraStartTransferTWL(u16 *buffer, u8 captureMode, u8 ndmaId);
bool cameraStartTransfer(u16 *buffer, u8 captureMode, u8 ndmaId)
{
    if (!isDSiMode())
        return false;
    return cameraStartTransferTWL(buffer, captureMode, ndmaId);
}
