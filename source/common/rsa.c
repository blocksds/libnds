// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

#include <nds/rsa.h>
#include <nds/system.h>

int swiRSAInitHeapTWL(swiRSAHeapContext_t *ctx, void *heapStart, size_t heapSize);
int swiRSADecryptRAWTWL(swiRSAHeapContext_t *ctx, swiRSAbuffers_t *rsabuffers, size_t len);
int swiRSADecryptTWL(swiRSAHeapContext_t *ctx, void *dst, const void *sig, const void *key);
int swiRSADecryptPGPTWL(swiRSAHeapContext_t *ctx, void *dst, const void *sig, const void *key);

int swiRSAInitHeap(swiRSAHeapContext_t *ctx, void *heapStart, size_t heapSize)
{
    if (isDSiMode())
        return swiRSAInitHeapTWL(ctx, heapStart, heapSize);
    return 0;
}

int swiRSADecryptRAW(swiRSAHeapContext_t *ctx, swiRSAbuffers_t *rsabuffers, size_t len)
{
    if (isDSiMode())
        return swiRSADecryptRAWTWL(ctx, rsabuffers, len);
    return 0;
}

int swiRSADecrypt(swiRSAHeapContext_t *ctx, void *dst, const void *sig, const void *key)
{
    if (isDSiMode())
        return swiRSADecryptTWL(ctx, dst, sig, key);
    return 0;
}

int swiRSADecryptPGP(swiRSAHeapContext_t *ctx, void *dst, const void *sig, const void *key)
{
    if (isDSiMode())
        return swiRSADecryptPGPTWL(ctx, dst, sig, key);
    return 0;
}
