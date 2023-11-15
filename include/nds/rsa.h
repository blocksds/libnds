// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

#ifndef LIBNDS_NDS_RSA_H__
#define LIBNDS_NDS_RSA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include <nds/ndstypes.h>

typedef struct swiRSAHeapContext {
    void *heapStart;
    void *heapEnd;
    size_t heapSize;
} swiRSAHeapContext_t;

typedef struct swiRSAbuffers {
    void *dst;
    const void *sig;
    const void *key;
} swiRSAbuffers_t;

int swiRSAInitHeap(swiRSAHeapContext_t *ctx, void *heapStart, size_t heapSize);
int swiRSADecryptRAW(swiRSAHeapContext_t *ctx, swiRSAbuffers_t *rsabuffers, size_t len);
int swiRSADecrypt(swiRSAHeapContext_t *ctx, void *dst, const void *sig, const void *key);
int swiRSADecryptPGP(swiRSAHeapContext_t *ctx, void *dst, const void *sig, const void *key);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_RSA_H__
