// SPDX-License-Identifier: BSD-3-Clause
//
// Copyright (c) 2006 Michael "Chishm" Chisholm

// Interface template for low level disc functions. Based on code originally
// written by MightyMax.

#ifndef LIBNDS_NDS_DISC_IO_H__
#define LIBNDS_NDS_DISC_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

#define FEATURE_MEDIUM_CANREAD      0x00000001
#define FEATURE_MEDIUM_CANWRITE     0x00000002
#define FEATURE_SLOT_GBA            0x00000010 // This is a slot-2 flashcard
#define FEATURE_SLOT_NDS            0x00000020 // This is a slot-1 flashcart
#define FEATURE_ARM7_CAPABLE        0x00000100 // It can be used from ARM7 and ARM9

#define DEVICE_TYPE_DSI_SD          ('_') | ('S' << 8) | ('D' << 16) | ('_' << 24)

typedef bool (*FN_MEDIUM_STARTUP)(void);
typedef bool (*FN_MEDIUM_ISINSERTED)(void);
typedef bool (*FN_MEDIUM_READSECTORS)(sec_t sector, sec_t numSectors, void *buffer);
typedef bool (*FN_MEDIUM_WRITESECTORS)(sec_t sector, sec_t numSectors, const void *buffer);
typedef bool (*FN_MEDIUM_CLEARSTATUS)(void);
typedef bool (*FN_MEDIUM_SHUTDOWN)(void);

struct DISC_INTERFACE_STRUCT {
    unsigned long           ioType;
    unsigned long           features;
    FN_MEDIUM_STARTUP       startup;
    FN_MEDIUM_ISINSERTED    isInserted;
    FN_MEDIUM_READSECTORS   readSectors;
    FN_MEDIUM_WRITESECTORS  writeSectors;
    FN_MEDIUM_CLEARSTATUS   clearStatus;
    FN_MEDIUM_SHUTDOWN      shutdown;
} ;

typedef struct DISC_INTERFACE_STRUCT DISC_INTERFACE;

const DISC_INTERFACE *get_io_dsisd(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_DISC_IO_H__
