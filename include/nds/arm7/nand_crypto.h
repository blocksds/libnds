// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Edoardo Lolletti (edo9300)

#ifndef LIBNDS_NDS_ARM7_NAND_CRYPTO_H__
#define LIBNDS_NDS_ARM7_NAND_CRYPTO_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARM7
#error NAND Crypto is only available on the ARM7
#endif

/// @file nds/arm7/nand_crypto.h
///
/// @brief Low-level NAND cryptographic functions for the ARM7

#include <nds/ndstypes.h>

/// Initializes the Keyslot 3 and the internal state
/// required to perform cryptographic operations on the NAND.
void nandCrypt_Init(void);

/// Checks if nand crypto initialization has been performed.
///
/// @return
///     If the AES keyslots were initialized, returns true.
bool nandCrypt_Initialized(void);

/// Sets the Input Vector in the aes engine to crypt the NAND blocks.
///
/// @param offset
///     16-byte block offset in the NAND flash where the ctypt operation will start at.
void nandCrypt_SetIV(u32 offset);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM7_NAND_CRYPTO_H__
