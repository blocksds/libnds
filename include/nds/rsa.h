// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

// DSi RSA functions

/// @file nds/rsa.h
///
/// @brief DSi RSA functions.

#ifndef LIBNDS_NDS_RSA_H__
#define LIBNDS_NDS_RSA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <nds/ndstypes.h>

#define SWI_RSA_DEFAULT_HEAP_SIZE 4096

/// Context that holds information about the RSA heap
typedef struct swiRSAHeapContext
{
    void *heapStart ALIGN(4); ///< Start of the heap rounded up to 4-byte boundary
    void *heapEnd ALIGN(4);   ///< start + size rounded down to 4-byte boundary
    size_t heapSize;          ///< Heap size matched to the above rounded values
} swiRSAHeapContext_t;

/// Struture that holds pointers to the RSA buffers
typedef struct swiRSAbuffers
{
    void *dst;          ///< Pointer to the output buffer
    const void *sig;    ///< Pointer to the signature buffer
    const void *key;    ///< Pointer to the RSA key buffer
} swiRSAbuffers_t;

/// Initialize RSA Heap
///
/// @param ctx
///     12-byte heap information structure that gets set with heap start, heap
///     end and heap length.
/// @param heapStart
///     Pointer to the start of the heap.
/// @param heapSize
///     Size of the heap in bytes (should be usually 4096).
///
/// @return
///     1 if success, 0 if failed.
int swiRSAInitHeap(swiRSAHeapContext_t *ctx, void *heapStart, size_t heapSize);

/// Decrypt signature using the owner's public key and return the hash of the
/// data that the signature belongs to. This function does not remove padding
/// from the output data but leading 0x00 bytes are stripped.
///
/// @param ctx
///     RSA Heap context
/// @param rsabuffers
///     Struct containing pointers to destination buffer, signature and public
///     key to use.
/// @param lenDst
///     Pointer to memory where to store the length of the destination buffer.
///
/// @return
///     1 if success, 0 if failed.
int swiRSADecryptRAW(swiRSAHeapContext_t *ctx, swiRSAbuffers_t *rsabuffers, size_t *lenDst);

/// Decrypt signature and also remove padding.
///
/// @param ctx
///     RSA Heap context
/// @param dst
///     Pointer to output buffer (should be 128 bytes).
/// @param sig
///     Pointer to signature buffer (should be 128 bytes).
/// @param key
///     Pointer to key buffer (should be 128 bytes).
///
/// @return
///     1 if success, 0 if failed
int swiRSADecrypt(swiRSAHeapContext_t *ctx, void *dst, const void *sig, const void *key);

/// Decrypt signature and extract the SHA1 value from a OpenPGP header.
///
/// @param ctx
///     RSA Heap context
/// @param dst
///     Pointer to output buffer.
/// @param sig
///     Pointer to signature buffer.
/// @param key
///     Pointer to key buffer.
///
/// @return
///     1 if success, 0 if failed
int swiRSADecryptPGP(swiRSAHeapContext_t *ctx, void *dst, const void *sig, const void *key);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_RSA_H__
