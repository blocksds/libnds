// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (c) 2006 Michael Chisholm (Chishm) and Tim Seidel (Mighty Max).

#ifndef LIBNDS_NDS_DISC_IO_H__
#define LIBNDS_NDS_DISC_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <nds/ndstypes.h>

#define FEATURE_MEDIUM_CANREAD      0x00000001 ///< This driver can be used to read sectors.
#define FEATURE_MEDIUM_CANWRITE     0x00000002 ///< This driver can be used to write sectors.
#define FEATURE_SLOT_GBA            0x00000010 ///< This driver uses Slot-2 cartridges.
#define FEATURE_SLOT_NDS            0x00000020 ///< This driver uses Slot-1 cartridges.
#define FEATURE_ARM7_CAPABLE        0x00000100 ///< This driver can be safely used from ARM7 and ARM9. BlocksDS extension.

#define DEVICE_TYPE_DSI_SD          ('_') | ('S' << 8) | ('D' << 16) | ('_' << 24)

typedef bool (*FN_MEDIUM_STARTUP)(void);
typedef bool (*FN_MEDIUM_ISINSERTED)(void);
typedef bool (*FN_MEDIUM_READSECTORS)(sec_t sector, sec_t numSectors, void *buffer);
typedef bool (*FN_MEDIUM_WRITESECTORS)(sec_t sector, sec_t numSectors, const void *buffer);
typedef bool (*FN_MEDIUM_CLEARSTATUS)(void);
typedef bool (*FN_MEDIUM_SHUTDOWN)(void);

typedef struct DISC_INTERFACE_STRUCT
{
    /// Four-byte identifier of the device type implemented by this interface.
    unsigned long           ioType;

    /// Available device features.
    ///
    /// @see FEATURE_MEDIUM_CANREAD
    /// @see FEATURE_MEDIUM_CANWRITE
    /// @see FEATURE_SLOT_GBA
    /// @see FEATURE_SLOT_NDS
    /// @see FEATURE_ARM7_CAPABLE
    unsigned long           features;

    /// Initialize the device.
    ///
    /// @return
    ///     True on success.
    FN_MEDIUM_STARTUP       startup;

    /// Check if the device's removable storage, if any, is inserted.
    ///
    /// @return
    ///     True if storage is available.
    FN_MEDIUM_ISINSERTED    isInserted;

    /// Read sectors from the device.
    ///
    /// Sectors are assumed to always be 512 bytes in size. Note that some
    /// drivers only support aligned buffers.
    ///
    /// @param sector
    ///     The sector number.
    /// @param numSectors
    ///     The number of sectors.
    /// @param buffer
    ///     The destination buffer.
    ///
    /// @return
    ///     True on success.
    FN_MEDIUM_READSECTORS   readSectors;

    /// Write sectors to the device.
    ///
    /// Sectors are assumed to always be 512 bytes in size. Note that some
    /// drivers only support aligned buffers.
    ///
    /// @param sector
    ///     The sector number.
    /// @param numSectors
    ///     The number of sectors.
    /// @param buffer
    ///     The source buffer.
    ///
    /// @return
    ///     True on success.
    FN_MEDIUM_WRITESECTORS  writeSectors;

    /// Reset the device's error status after an error occured.
    ///
    /// This is not used by applications. Drivers are expected to do this
    /// automatically.
    ///
    /// @return
    ///     True on success.
    FN_MEDIUM_CLEARSTATUS   clearStatus;

    /// Shut down the device.
    ///
    /// @return
    ///     True on success.
    FN_MEDIUM_SHUTDOWN      shutdown;
} DISC_INTERFACE;

/// Return the internal DSi SD card interface.
WARN_UNUSED_RESULT
const DISC_INTERFACE *get_io_dsisd(void);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_DISC_IO_H__
