// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

#ifndef LIBNDS_ARM9_DLDI_ASM_H__
#define LIBNDS_ARM9_DLDI_ASM_H__

#define FEATURE_MEDIUM_CANREAD      0x00000001
#define FEATURE_MEDIUM_CANWRITE     0x00000002
#define FEATURE_SLOT_GBA            0x00000010 // This is a slot-2 flashcard
#define FEATURE_SLOT_NDS            0x00000020 // This is a slot-1 flashcart
#define FEATURE_ARM7_CAPABLE        0x00000100 // It can be used from ARM7 and ARM9

#define FIX_ALL         0x01
#define FIX_GLUE        0x02
#define FIX_GOT         0x04
#define FIX_BSS         0x08

#define DLDI_SIZE_32KB  0x0f
#define DLDI_SIZE_16KB  0x0e
#define DLDI_SIZE_8KB   0x0d
#define DLDI_SIZE_4KB   0x0c
#define DLDI_SIZE_2KB   0x0b
#define DLDI_SIZE_1KB   0x0a

#endif // LIBNDS_ARM9_DLDI_ASM_H__
