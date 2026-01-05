// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2026 Adrian "asie" Siekierka

#pragma GCC optimize("-Os")

#ifdef DEBUG
# define FOOTERS 1
# define INSECURE 0
#else
# define FOOTERS 0
# define INSECURE 1
#endif

#define HAVE_MORECORE 1
#define HAVE_MMAP 0
#define malloc_getpagesize 4096

#undef DEBUG
#include "dlmalloc_impl.h"

void *aligned_alloc(size_t, size_t) __attribute__((alias("memalign"), leaf, nothrow));
void *__malloc_malloc(size_t) __attribute__((alias("malloc"), leaf, malloc, nothrow));
void __malloc_free(void *) __attribute__((alias("free"), leaf, nothrow));
void cfree(void *) __attribute__((alias("free"), leaf, nothrow));
