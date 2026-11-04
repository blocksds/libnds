// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025-2026 Antonio Niño Díaz

#ifndef LIBNDS_NDS_COTRHEAD_ASM_H__
#define LIBNDS_NDS_COTRHEAD_ASM_H__

// This file must only have definitions that can be used from assembly files.

// Invalid ID for signals. It must have bit 31 set to 1 so that it's a system
// ID, not a user ID.
#define COTHREAD_NO_SIGNAL 0xFFFFFFFF

/// Flags a thread as detached.
///
/// A detached thread deallocates all memory used by it when it ends. Calling
/// cothread_has_joined() or cothread_get_exit_code() isn't allowed.
#define COTHREAD_DETACHED   (1 << 0)

/// Flags a thread as waiting for an event (like an interrupt)
#define COTHREAD_WAITING    (1 << 1)

// Offsets to fields inside the cothread_info_t struct
#define COTHREAD_INFO_NEXT_OFFSET               16
#define COTHREAD_INFO_FLAGS_OFFSET              20
#define COTHREAD_INFO_WAIT_IRQ_MASK_OFFSET      28

#endif // LIBNDS_NDS_COTRHEAD_ASM_H__
