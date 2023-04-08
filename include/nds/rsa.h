// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

#ifndef RSA_H_INCLUDE
#define RSA_H_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#include "nds/ndstypes.h"
#include <stddef.h>

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


#endif // RSA_H_INCLUDE
