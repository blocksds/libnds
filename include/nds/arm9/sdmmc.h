// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_ARM9_SDMMC_H__
#define LIBNDS_NDS_ARM9_SDMMC_H__

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// These values should be synchronized with <fatfs/diskio.h>.
#define SDMMC_STATUS_NOINIT		0x01	/* Drive not initialized */
#define SDMMC_STATUS_NODISK		0x02	/* No medium in the drive */
#define SDMMC_STATUS_PROTECT    0x04	/* Write protected */

#define SDMMC_DEVICE_SD         0
#define SDMMC_DEVICE_NAND       1

/**
 * @brief      Initialize the eMMC NAND.
 *
 * @return     Returns true on success or false on failure.
 */
bool nand_Startup(void);

/**
 * @brief      Returns the SDMMC_STATUS bits of the eMMC NAND.
 *
 * @return     Returns the SDMMC_STATUS bits or SDMMC_STATUS_NODISK | SDMMC_STATUS_NOINIT on failure.
 */
u8 nand_GetDiskStatus(void);

/**
 * @brief      Returns the SDMMC_STATUS bits of the SD card.
 *
 * @return     Returns the SDMMC_STATUS bits or SDMMC_STATUS_NODISK | SDMMC_STATUS_NOINIT on failure.
 */
u8 sdmmc_GetDiskStatus(void);

/**
 * @brief      Outputs the number of sectors for the eMMC NAND.
 *
 * @return     Returns the number of sectors or 0 on failure.
 */
u32 nand_GetSectors(void);

/**
 * @brief      Outputs the number of sectors for the SD card.
 *
 * @return     Returns the number of sectors or 0 on failure.
 */
u32 sdmmc_GetSectors(void);

/**
 * @brief      Reads one or more sectors from the eMMC NAND.
 *
 * @param[in]  sector      The start sector.
 * @param[in]  numSectors  The number of sectors to read.
 * @param      buffer      The output buffer pointer.
 *
 * @return     Returns true on success or false on failure.
 */
bool nand_ReadSectors(sec_t sector, sec_t numSectors, void *buffer);

/**
 * @brief      Writes one or more sectors to the eMMC NAND.
 *
 * @param[in]  sector      The start sector.
 * @param[in]  numSectors  The number of sectors to read.
 * @param[in]  buffer      The input buffer pointer.
 *
 * @return     Returns true on success or false on failure.
 */
bool nand_WriteSectors(sec_t sector, sec_t numSectors, const void *buffer);

// Compatibility macros.
#define nand_GetSize nand_GetSectors

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_SDMMC_H__
