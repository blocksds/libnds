// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version (https://github.com/profi200/dsi_sdmmc)
//
// Copyright (C) 2023 profi200

#ifndef LIBNDS_NDS_ARM7_SDMMC_H__
#define LIBNDS_NDS_ARM7_SDMMC_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM7
#error SDMMC header is for ARM7 only
#endif

#include <nds/arm7/tmio.h>
#include <nds/mmc/mmc_spec.h>
#include <nds/mmc/sd_spec.h>

// Possible error codes for most of the functions below.
enum
{
    SDMMC_ERR_NONE             =  0u, // No error.
    SDMMC_ERR_INVAL_PARAM      =  1u, // Invalid parameter.
    SDMMC_ERR_INITIALIZED      =  2u, // The device is already initialized.
    SDMMC_ERR_GO_IDLE_STATE    =  3u, // GO_IDLE_STATE CMD error.
    SDMMC_ERR_SEND_IF_COND     =  4u, // SEND_IF_COND CMD error.
    SDMMC_ERR_IF_COND_RESP     =  5u, // IF_COND response pattern mismatch or unsupported voltage.
    SDMMC_ERR_SEND_OP_COND     =  6u, // SEND_OP_COND CMD error.
    SDMMC_ERR_OP_COND_TMOUT    =  7u, // Card initialization timeout.
    SDMMC_ERR_VOLT_SUPPORT     =  8u, // Voltage not supported.
    SDMMC_ERR_ALL_SEND_CID     =  9u, // ALL_SEND_CID CMD error.
    SDMMC_ERR_SET_SEND_RCA     = 10u, // SET/SEND_RELATIVE_ADDR CMD error.
    SDMMC_ERR_SEND_CSD         = 11u, // SEND_CSD CMD error.
    SDMMC_ERR_SELECT_CARD      = 12u, // SELECT_CARD CMD error.
    SDMMC_ERR_LOCKED           = 13u, // Card is locked with a password.
    SDMMC_ERR_SEND_EXT_CSD     = 14u, // SEND_EXT_CSD CMD error.
    SDMMC_ERR_SWITCH_HS        = 15u, // Error on switching to high speed mode.
    SDMMC_ERR_SET_CLR_CD       = 16u, // SET_CLR_CARD_DETECT CMD error.
    SDMMC_ERR_SET_BUS_WIDTH    = 17u, // Error on switching to a different bus width.
    SDMMC_ERR_SEND_STATUS      = 18u, // SEND_STATUS CMD error.
    SDMMC_ERR_CARD_STATUS      = 19u, // The card returned an error via its status.
    SDMMC_ERR_NO_CARD          = 20u, // Card unitialized or not inserted.
    SDMMC_ERR_SECT_RW          = 21u, // Sector read/write error.
    SDMMC_ERR_WRITE_PROT       = 22u, // The card is write protected.
    SDMMC_ERR_SEND_CMD         = 23u, // An error occured while sending a custom CMD via SDMMC_sendCommand().
    SDMMC_ERR_SET_BLOCKLEN     = 24u, // SET_BLOCKLEN CMD error.
    SDMMC_ERR_LOCK_UNLOCK      = 25u, // LOCK_UNLOCK CMD error.
    SDMMC_ERR_LOCK_UNLOCK_FAIL = 26u, // Lock/unlock operation failed (R1 status).
    SDMMC_ERR_SLEEP_AWAKE      = 27u  // (e)MMC SLEEP_AWAKE CMD error.
};

// (e)MMC/SD device numbers.
enum
{
    SDMMC_DEV_CARD = 0u, // SD card/MMC.
    SDMMC_DEV_eMMC = 1u, // Builtin eMMC.

    // Alias for internal use only.
    SDMMC_MAX_DEV_NUM = SDMMC_DEV_eMMC
};

// Bit definition for SdmmcInfo.prot.
// Each bit 1 = protected.
#define SDMMC_PROT_SLIDER    (1u)    // SD card write protection slider.
#define SDMMC_PROT_TEMP      (1u<<1) // Temporary write protection (CSD).
#define SDMMC_PROT_PERM      (1u<<2) // Permanent write protection (CSD).
#define SDMMC_PROT_PASSWORD  (1u<<3) // (e)MMC/SD card is password protected.

typedef struct
{
    u8 type;     // 0 = none, 1 = (e)MMC, 2 = High capacity (e)MMC, 3 = SDSC, 4 = SDHC/SDXC, 5 = SDUC.
    u8 prot;     // See SDMMC_PROT_... defines above for details.
    u16 rca;     // Relative Card Address (RCA).
    u32 sectors; // Size in 512 byte units.
    u32 clock;   // The current clock frequency in Hz.
    u32 cid[4];  // Raw CID without the CRC.
    u16 ccc;     // (e)MMC/SD command class support from CSD. One per bit starting at 0.
    u8 busWidth; // The current bus width used to talk to the card.
} SdmmcInfo;

typedef struct
{
    u16 cmd;     // Command. The format is controller specific!
    u32 arg;     // Command argument.
    u32 resp[4]; // Card response. Length depends on command.
    u32 *buf;    // In/out data buffer.
    u16 blkLen;  // Block length. Usually 512.
    u16 count;   // Number of blkSize blocks to transfer.
} MmcCommand;

// Mode bits for SDMMC_lockUnlock().
#define SDMMC_LK_CLR_PWD  (1u << 1) // Clear password.
#define SDMMC_LK_UNLOCK   (0u)      // Unlock.
#define SDMMC_LK_LOCK     (1u << 2) // Lock.
#define SDMMC_LK_ERASE    (1u << 3) // Force erase a locked (e)MMC/SD card.
#define SDMMC_LK_COP      (1u << 4) // SD cards only. Card Ownership Protection operation.

// These values should be synchronized with <fatfs/diskio.h>.
#define SDMMC_STATUS_NOINIT     0x01 // Drive not initialized
#define SDMMC_STATUS_NODISK     0x02 // No medium in the drive
#define SDMMC_STATUS_PROTECT    0x04 // Write protected

/// Initializes a (e)MMC/SD card device.
///
/// @param devNum
///     The device to initialize.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or one of the errors listed above on
///     failure.
u32 SDMMC_init(const u8 devNum);

/// Switches a (e)MMC/SD card device between sleep/awake mode.
///
/// Note that SD cards don't have a true sleep mode.
///
/// @param devNum
///     The device.
/// @param enabled
///     The mode. true to enable sleep and false to wake up.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or one of the errors listed above on
///     failure.
u32 SDMMC_setSleepMode(const u8 devNum, const bool enabled);

/// Deinitializes a (e)MMC/SD card device.
///
/// @param devNum
///     The device to deinitialize.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or SDMMC_ERR_INVAL_PARAM on failure.
u32 SDMMC_deinit(const u8 devNum);

/// Manage password protection for a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
/// @param mode
///     The mode of operation. See defines above.
/// @param pwd
///     The password buffer pointer.
/// @param pwdLen
///     The password length. Maximum 32 for password replace.  Otherwise 16.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or one of the errors listed above on
///     failure.
u32 SDMMC_lockUnlock(const u8 devNum, const u8 mode, const u8 *const pwd, const u8 pwdLen);

/// Exports the internal device state for fast init (bootloaders ect.).
///
/// @param devNum
///     The device state to export.
/// @param devOut
///     A pointer to a u8[60] array.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or
///     SDMMC_ERR_INVAL_PARAM/SDMMC_ERR_NO_CARD on failure.
u32 SDMMC_exportDevState(const u8 devNum, u8 devOut[64]);

/// Imports a device state for fast init (bootloaders ect.).
///
/// The state should be validated for example with a checksum.
///
/// @param devNum
///     The device state to import.
/// @param devIn
///     A pointer to a u8[60] array.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or
///     SDMMC_ERR_INVAL_PARAM/SDMMC_ERR_NO_CARD/SDMMC_ERR_INITIALIZED on
///     failure.
u32 SDMMC_importDevState(const u8 devNum, const u8 devIn[64]);

/// Outputs infos about a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
/// @param infoOut
///     A pointer to a SdmmcInfo struct.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or SDMMC_ERR_INVAL_PARAM on failure.
u32 SDMMC_getDevInfo(const u8 devNum, SdmmcInfo *const infoOut);

/// Outputs the parsed CID of a (e)MMC/SD card device, in the format used by other OSes and drivers.
///
/// @param devNum
///     The device.
/// @param cidOut
///     A u32[4] pointer for storing the CID.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or SDMMC_ERR_INVAL_PARAM on failure.
u32 SDMMC_getCid(const u8 devNum, u32 cidOut[4]);

/// Outputs the raw CID of a (e)MMC/SD card device, as it is returned from the sdmmc controller.
/// This is the format used for the DSi NAND crypto.
///
/// @param devNum
///     The device.
/// @param cidOut
///     A u32[4] pointer for storing the raw CID.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or SDMMC_ERR_INVAL_PARAM on failure.
u32 SDMMC_getCidRaw(const u8 devNum, u32 cidOut[4]);

/// Returns the SDMMC_STATUS bits of a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
///
/// @return
///     Returns the SDMMC_STATUS bits or SDMMC_STATUS_NODISK |
///     SDMMC_STATUS_NOINIT on failure.
u8 SDMMC_getDiskStatus(const u8 devNum);

/// Outputs the number of sectors for a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
///
/// @return
///     Returns the number of sectors or 0 on failure.
u32 SDMMC_getSectors(const u8 devNum);

/// Reads one or more sectors from a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
/// @param sect
///     The start sector.
/// @param buf
///     The output buffer pointer. NULL for DMA.
/// @param count
///     The number of sectors to read.
/// @param crypt_callback
///     Callback function called each time a sector is processed.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or one of the errors listed above on
///     failure.
u32 SDMMC_readSectorsCrypt(const u8 devNum, u32 sect, void *const buf, const u16 count,
                           tmio_callback_t crypt_callback);

/// Reads one or more sectors from a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
/// @param sect
///     The start sector.
/// @param buf
///     The output buffer pointer. NULL for DMA.
/// @param count
///     The number of sectors to read.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or one of the errors listed above on
///     failure.
static inline u32 SDMMC_readSectors(const u8 devNum, u32 sect, void *const buf, const u16 count)
{
	return SDMMC_readSectorsCrypt(devNum, sect, buf, count, NULL);
}

/// Writes one or more sectors to a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
/// @param sect
///     The start sector.
/// @param buf
///     The input buffer pointer. NULL for DMA.
/// @param count
///     The count
/// @param crypt_callback
///     Callback function called each time a sector is processed.
///
/// @return
///     Returns SDMMC_ERR_NONE on success or one of the errors listed above on
///     failure.
u32 SDMMC_writeSectorsCrypt(const u8 devNum, u32 sect, const void *const buf,
                            const u16 count, tmio_callback_t crypt_callback);

/// Writes one or more sectors to a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
/// @param sect
///     The start sector.
/// @param buf
///     The input buffer pointer. NULL for DMA.
/// @param count
///     The count
///
/// @return
///     Returns SDMMC_ERR_NONE on success or one of the errors listed above on
///     failure.
static inline u32 SDMMC_writeSectors(const u8 devNum, u32 sect, const void *const buf,
                       const u16 count)
{
	return SDMMC_writeSectorsCrypt(devNum, sect, buf, count, NULL);
}

/// Sends a custom command to a (e)MMC/SD card device.
///
/// @param devNum
///     The device.
/// @param cmd
///     MMC command struct pointer (see above).
///
/// @return
///     Returns SDMMC_ERR_NONE on success or SDMMC_ERR_SEND_CMD on failure.
u32 SDMMC_sendCommand(const u8 devNum, MmcCommand *const mmcCmd);

/// Returns the R1 card status for a previously failed read/write/custom command.
///
/// @param devNum
///     The device.
///
/// @return
///     Returns the R1 card status or 0 if there was either no command error or
///     invalid devNum.
u32 SDMMC_getLastR1error(const u8 devNum);

// TODO: TRIM/erase support.

#ifdef __cplusplus
}
#endif

#endif
