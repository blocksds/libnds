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

#define DLDI_MAGIC_STRING_LEN   8
#define DLDI_FRIENDLY_NAME_LEN  48

extern const u32 DLDI_MAGIC_NUMBER;

/**
 * @brief DLDI I/O driver interface.
 */
typedef struct DLDI_INTERFACE {
    /**
     * @brief Magic number, equal to 0xBF8DA5ED.
     * 
     * @see dldiIsValid
     */
    u32 magicNumber;
    /**
     * @brief Magic string, equal to " Chishm\0".
     * 
     * @see dldiIsValid
     */
    char magicString [DLDI_MAGIC_STRING_LEN];
    /**
     * @brief Version number.
     */
    u8 versionNumber;
    /**
     * @brief Log-2 of the driver's size, in bytes.
     */
    u8 driverSize;
    /**
     * @brief Flags which determine the sections that may have
     * addresses to be fixed.
     *
     * @see FIX_ALL
     * @see FIX_GLUE
     * @see FIX_GOT
     * @see FIX_BSS
     */
    u8 fixSectionsFlags;
    /**
     * @brief Log-2 of the available maximum driver size, in bytes.
     */
    u8 allocatedSize;

    /**
     * @brief User-friendly driver name.
     */
    char friendlyName [DLDI_FRIENDLY_NAME_LEN];

    // Pointers to sections that need address fixing
    void *dldiStart; ///< Start of the DLDI driver's text/data section.
    void *dldiEnd; ///< End of the DLDI driver's text/data section.
    void *interworkStart; ///< Start of the DLDI driver's ARM interwork section.
    void *interworkEnd; ///< End of the DLDI driver's ARM interwork section.
    void *gotStart; ///< Start of the DLDI driver's Global Offset Table section.
    void *gotEnd; ///< End of the DLDI driver's Global Offset Table section.
    void *bssStart; ///< Start of the DLDI driver's BSS section.
    void *bssEnd; ///< End of the DLDI driver's BSS section.

    /**
     * @brief File system interface flags and functions.
     */
    DISC_INTERFACE ioInterface;
} DLDI_INTERFACE;

typedef enum {
    /**
     * @brief DLDI runtime mode: Look for FEATURE_ARM7_CAPABLE in DLDI header.
     */
    DLDI_MODE_AUTODETECT = -1,
    /**
     * @brief DLDI runtime mode: Always use the ARM9 CPU.
     */
    DLDI_MODE_ARM9 = 0,
    /**
     * @brief DLDI runtime mode: Always use the ARM7 CPU.
     */
    DLDI_MODE_ARM7 = 1,
} DLDI_MODE;

/**
 * @brief Pointer to the internal DLDI driver.
 *
 * Make sure to set the bus permissions appropriately before using.
 */
extern const DLDI_INTERFACE* io_dldi_data;

/**
 * @brief Set the DLDI runtime mode.
 *
 * This controls which CPU runs the DLDI driver's code.
 */
void dldiSetMode(DLDI_MODE mode);

/**
 * @brief Get the DLDI runtime mode.
 */
DLDI_MODE dldiGetMode(void);

/**
 * @brief Return a pointer to the internal IO interface and set up the bus
 * permissions.
 */
extern const DISC_INTERFACE* dldiGetInternal(void);

/**
 * @brief Determine if an IO driver is a valid DLDI driver.
 */
extern bool dldiIsValid(const DLDI_INTERFACE* io);

/**
 * @brief Adjust the pointer addresses within a DLDI driver.
 */
extern void dldiFixDriverAddresses(DLDI_INTERFACE* io);

/**
 * @brief Load a DLDI driver from a file and set up the bus permissions.
 * 
 * This is not directly usable as a filesystem driver.
 */
extern DLDI_INTERFACE *dldiLoadFromFile(const char* path);

/**
 * @brief Free the memory used by the DLDI driver.
 *
 * Remember to shut down the driver itself first:
 *
 *     loadedDldi->ioInterface.shutdown();
 *     dldiFree(loadedDldi);
 */
extern void dldiFree(DLDI_INTERFACE* dldi);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_DLDI_H__
