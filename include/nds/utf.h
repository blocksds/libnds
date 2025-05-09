// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025 Antonio Niño Díaz

#ifndef LIBNDS_NDS_UTF_H__
#define LIBNDS_NDS_UTF_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/utf.h
///
/// @brief UTF helpers.
///
/// Helpers used to handle different UTF formats.

#include <stddef.h>
#include <sys/types.h>
#include <uchar.h>

#include <nds/ndstypes.h>

/// It converts a UTF-16LE string to UTF-8.
///
/// This can be used for the firmware user setting strings, like the user name
/// and the personal message.
///
/// @param out
///     Destination buffer for the resulting string encoded as UTF-8.
/// @param out_size
///     Size of the destination buffer in bytes.
/// @param in
///     Source buffer of the UTF-16LE encoded string.
/// @param in
///     Size of the source buffer in bytes.
///
/// @result
///     It returns the number of saved bytes in the destination buffer on
///     success or a negative number on error. On error, the contents of the
///     output buffer shouldn't be used.
ssize_t utf16_to_utf8(char *out, size_t out_size, char16_t *in, size_t in_size);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_UTF_H__
