// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (c) 2006 Michael Chisholm (Chishm)

#ifndef LIBNDS_NDS_ARM9_DLDI_H__
#define LIBNDS_NDS_ARM9_DLDI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/arm9/dldi_asm.h>
#include <nds/disc_io.h>

#define FIX_ALL                 0x01
#define FIX_GLUE                0x02
#define FIX_GOT                 0x04
#define FIX_BSS                 0x08

#define DLDI_MAGIC_STRING_LEN   8
#define DLDI_FRIENDLY_NAME_LEN  48

extern const u32 DLDI_MAGIC_NUMBER;

// I/O interface with DLDI extensions
typedef struct DLDI_INTERFACE {
    u32 magicNumber;
    char magicString [DLDI_MAGIC_STRING_LEN];
    u8 versionNumber;
    u8 driverSize;       // Log-2 of driver size in bytes
    u8 fixSectionsFlags;
    u8 allocatedSize;    // Log-2 of the allocated space in bytes

    char friendlyName [DLDI_FRIENDLY_NAME_LEN];

    // Pointers to sections that need address fixing
    void *dldiStart;
    void *dldiEnd;
    void *interworkStart;
    void *interworkEnd;
    void *gotStart;
    void *gotEnd;
    void *bssStart;
    void *bssEnd;

    // Original I/O interface data
    DISC_INTERFACE ioInterface;
} DLDI_INTERFACE;

typedef enum {
    DLDI_MODE_AUTODETECT = -1, // Look for FEATURE_ARM7_CAPABLE in DLDI header
    DLDI_MODE_ARM9 = 0,
    DLDI_MODE_ARM7 = 1,
} DLDI_MODE;

// Pointer to the internal DLDI, not directly usable in libfat. You'll need to
// set the bus permissions appropriately before using.
extern const DLDI_INTERFACE* io_dldi_data;

// Set DLDI runtime mode.
void dldiSetMode(DLDI_MODE mode);

// Get DLDI runtime mode.
DLDI_MODE dldiGetMode(void);

// Return a pointer to the internal IO interface, setting up bus permissions in
// the process.
extern const DISC_INTERFACE* dldiGetInternal(void);

// Determines if an IO driver is a valid DLDI driver.
extern bool dldiIsValid(const DLDI_INTERFACE* io);

// Adjust the pointer addresses within a DLDI driver.
extern void dldiFixDriverAddresses(DLDI_INTERFACE* io);

// Load a DLDI from disc and set up the bus permissions. This returns a type not
// directly usable in libfat, but it does give extra information, such as a
// friendly name.
//
// To use in libfat:
//
//     const DLDI_INTERFACE *loadedDldi = dldi_loadFromFile("file");
//     loadedDldi->ioInterface.startup();
//     fatMount(&loadedDldi->ioInterface, "devname", 0);
extern DLDI_INTERFACE *dldiLoadFromFile(const char* path);

// Free resources used by a loaded DLDI.
//
// Remember to unmount and shutdown first:
//
//     fatUnmount("devname");
//     loadedDldi->ioInterface.shutdown();
//     dldiFree(loadedDldi);
extern void dldiFree (DLDI_INTERFACE* dldi);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_DLDI_H__
