// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

// DSi sha1 functions

/*!	\file sha1.h
	\brief DSi SHA1 functions.
*/

#ifndef SHA1_H_INCLUDE
#define SHA1_H_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#include "nds/ndstypes.h"
#include <stddef.h>

typedef struct swiSHA1context {
	u32 state[5];       /*!< intermediate digest state  */
	u32 total[2];       /*!< number of bytes processed  */
	u8  buffer[64];     /*!< data block being processed */
	u32 fragment_size;
	void (*sha_block)(struct swiSHA1context *ctx, const void *src, size_t len); /*!< data block being processed */
} swiSHA1context_t;

/**
 * \brief          SHA-1 context setup
 *
 * \param ctx      context to be initialized
 */
void swiSHA1Init(swiSHA1context_t *ctx);

/**
 * \brief          SHA-1 process buffer
 *
 * \param ctx      SHA-1 context
 * \param data     buffer to process
 * \param len      length of data
 */
void swiSHA1Update(swiSHA1context_t *ctx, const void *data, size_t len);

/**
 * \brief          SHA-1 final digest
 *
 * \param digest   buffer to hold SHA-1 checksum result
 * \param ctx      SHA-1 context
 */
void swiSHA1Final(void *digest, swiSHA1context_t *ctx);

/**
 * \brief          SHA-1 checksum
 *
 * \param digest   buffer to hold SHA-1 checksum result
 * \param data     buffer to process
 * \param len      length of data
 */
void swiSHA1Calc(void *digest, const void *data, size_t len);

/**
 * \brief          SHA-1 verify
 *
 * \param digest1  buffer containing hash to verify
 * \param digest2  buffer containing hash to verify
 */
void swiSHA1Verify(const void *digest1, const void *digest2);

#ifdef __cplusplus
}
#endif


#endif // SHA1_H_INCLUDE
