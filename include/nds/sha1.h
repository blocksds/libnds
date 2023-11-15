// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

// DSi sha1 functions

/// @file nds/sha1.h
///
/// @brief DSi SHA1 functions.

#ifndef LIBNDS_NDS_SHA1_H__
#define LIBNDS_NDS_SHA1_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <nds/ndstypes.h>

typedef struct swiSHA1context {
    u32 state[5];       ///< Intermediate digest state
    u32 total[2];       ///< Number of bytes processed
    u8  buffer[64];     ///< Data block being processed
    u32 fragment_size;
    void (*sha_block)(struct swiSHA1context *ctx, const void *src, size_t len); ///< Data block being processed
} swiSHA1context_t;

/// SHA-1 context setup.
///
/// @param ctx Context to be initialized
void swiSHA1Init(swiSHA1context_t *ctx);

/// SHA-1 process buffer
///
/// @param ctx SHA-1 context.
/// @param data Buffer to process.
/// @param len Length of data.
void swiSHA1Update(swiSHA1context_t *ctx, const void *data, size_t len);

/// SHA-1 final digest
///
/// @param digest Buffer to hold SHA-1 checksum result
/// @param ctx SHA-1 context
void swiSHA1Final(void *digest, swiSHA1context_t *ctx);

/// SHA-1 checksum
///
/// @param digest Buffer to hold SHA-1 checksum result.
/// @param data Buffer to process.
/// @param len Length of data.
void swiSHA1Calc(void *digest, const void *data, size_t len);

/// SHA-1 verify
///
/// @param digest1 Buffer containing hash to verify.
/// @param digest2 Buffer containing hash to verify.
void swiSHA1Verify(const void *digest1, const void *digest2);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_SHA1_H__
