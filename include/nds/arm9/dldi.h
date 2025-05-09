// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2006 Michael Chisholm (Chishm)

#ifndef LIBNDS_NDS_ARM9_DLDI_H__
#define LIBNDS_NDS_ARM9_DLDI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/arm9/dldi_asm.h>
#include <nds/disc_io.h>

#define DLDI_MAGIC_STRING_LEN   8
#define DLDI_FRIENDLY_NAME_LEN  48

extern const u32 DLDI_MAGIC_NUMBER;

/// DLDI I/O driver interface.
typedef struct DLDI_INTERFACE
{
    /// Magic number, equal to 0xBF8DA5ED.
    ///
    /// @see dldiIsValid
    u32 magicNumber;

    /// Magic string, equal to " Chishm\0".
    ///
    /// @see dldiIsValid
    char magicString[DLDI_MAGIC_STRING_LEN];

    /// Version number.
    u8 versionNumber;

    /// Log-2 of the driver's size, in bytes.
    u8 driverSize;

    /// Flags which determine the sections that may have addresses to be fixed.
    ///
    /// @see FIX_ALL
    /// @see FIX_GLUE
    /// @see FIX_GOT
    /// @see FIX_BSS
    u8 fixSectionsFlags;

    /// Log-2 of the available maximum driver size, in bytes.
    u8 allocatedSize;

    /// User-friendly driver name.
    char friendlyName[DLDI_FRIENDLY_NAME_LEN];

    // Pointers to sections that need address fixing
    void *dldiStart; ///< Start of the DLDI driver's text/data section.
    void *dldiEnd; ///< End of the DLDI driver's text/data section.
    void *interworkStart; ///< Start of the DLDI driver's ARM interwork section.
    void *interworkEnd; ///< End of the DLDI driver's ARM interwork section.
    void *gotStart; ///< Start of the DLDI driver's Global Offset Table section.
    void *gotEnd; ///< End of the DLDI driver's Global Offset Table section.
    void *bssStart; ///< Start of the DLDI driver's BSS section.
    void *bssEnd; ///< End of the DLDI driver's BSS section.

    /// File system interface flags and functions.
    DISC_INTERFACE ioInterface;
} DLDI_INTERFACE;

typedef enum
{
    /// DLDI runtime mode: Look for FEATURE_ARM7_CAPABLE in DLDI header.
    DLDI_MODE_AUTODETECT = -1,

    /// DLDI runtime mode: Always use the ARM9 CPU.
    DLDI_MODE_ARM9 = 0,

    /// DLDI runtime mode: Always use the ARM7 CPU.
    DLDI_MODE_ARM7 = 1,
} DLDI_MODE;

/// Pointer to the internal DLDI driver.
///
/// Make sure to set the bus permissions appropriately before using.
extern const DLDI_INTERFACE *io_dldi_data;

/// Set the DLDI runtime mode.
///
/// This controls which CPU runs the DLDI driver's code.
void dldiSetMode(DLDI_MODE mode);

/// Get the DLDI runtime mode.
DLDI_MODE dldiGetMode(void);

/// Return a pointer to the internal IO interface and set up the bus
/// permissions.
const DISC_INTERFACE *dldiGetInternal(void);

/// Determine if an IO driver is a valid DLDI driver.
bool dldiIsValid(const DLDI_INTERFACE *io);

/// Relocate DLDI driver to a given target location in memory.
void dldiRelocate(DLDI_INTERFACE *io, void *targetAddress);

/// Adjust the pointer addresses within a DLDI driver.
static inline void dldiFixDriverAddresses(DLDI_INTERFACE *io)
{
    dldiRelocate(io, io);
}

/// Load a DLDI driver from a file and set up the bus permissions.
///
/// This is not directly usable as a filesystem driver.
DLDI_INTERFACE *dldiLoadFromFile(const char *path);

/// Free the memory used by the DLDI driver.
///
/// Remember to shut down the driver itself first:
///
/// ```
/// loadedDldi->ioInterface.shutdown();
/// dldiFree(loadedDldi);
/// ```
void dldiFree(DLDI_INTERFACE *dldi);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_DLDI_H__
