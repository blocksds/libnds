// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2017 Dave Murphy (WinterMute)

#include <nds/sha1.h>
#include <nds/system.h>

void swiSHA1InitTWL(swiSHA1context_t *ctx);
void swiSHA1UpdateTWL(swiSHA1context_t *ctx, const void *data, size_t len);
void swiSHA1FinalTWL(void *digest, swiSHA1context_t *ctx);
void swiSHA1CalcTWL(void *digest, const void *data, size_t len);
void swiSHA1VerifyTWL(const void *digest1, const void *digest2);

void swiSHA1Init(swiSHA1context_t *ctx)
{
    if (isDSiMode())
        swiSHA1InitTWL(ctx);
}

void swiSHA1Update(swiSHA1context_t *ctx, const void *data, size_t len)
{
    if (isDSiMode())
        swiSHA1UpdateTWL(ctx, data, len);
}

void swiSHA1Final(void *digest, swiSHA1context_t *ctx)
{
    if (isDSiMode())
        swiSHA1FinalTWL(digest, ctx);
}

void swiSHA1Calc(void *digest, const void *data, size_t len)
{
    if (isDSiMode())
        swiSHA1CalcTWL(digest, data, len);
}

void swiSHA1Verify(const void *digest1, const void *digest2)
{
    if (isDSiMode())
        swiSHA1VerifyTWL(digest1, digest2);
}
