// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Adrian "asie" Siekierka
// Copyright (c) 2024 Antonio Niño Díaz

#include <stdbool.h>

#include <nds/system.h>

bool slot2DetectTWLDebugRam(void)
{
    if (isHwDebugger())
        return true;

    return false;
}
