// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2025-2026 Antonio Niño Díaz

#ifndef LIBNDS_NDS_COTRHEAD_ASM_H__
#define LIBNDS_NDS_COTRHEAD_ASM_H__

// This file must only have definitions that can be used from assembly files.

// Invalid ID for signals. It must have bit 31 set to 1 so that it's a system
// ID, not a user ID.
#define COTHREAD_NO_SIGNAL 0xFFFFFFFF

// User flags
// ----------

/// Flags a thread as detached.
///
/// A detached thread deallocates all memory used by it when it ends. Calling
/// cothread_has_joined() or cothread_get_exit_code() isn't allowed.
#define COTHREAD_DETACHED   (1 << 0)

// Wait flags
// ----------

// Internal: Marks a thread as waiting for an event (like an interrupt)
#define COTHREAD_WAITING    (1 << 0)
// Note: Currently, the whole 'flags_wait' field is cleared by the IRQ handler.
// If more flags are added to it, that code needs to be modified too.

// Resources flags
// ---------------

// The following flags are used to remember which resources have been allocated
// by the cothread code. If they aren't set, the user has allocated them.
#define COTHREAD_ALLOCATED_CONTEXT (1 << 0)
#define COTHREAD_ALLOCATED_STACK   (1 << 1)

// cothread_info_t
// ---------------

// Offsets to fields inside the cothread_info_t struct
#define COTHREAD_INFO_STACK_POINTER_OFFSET      0
#define COTHREAD_INFO_EXIT_CODE_OFFSET          4
#define COTHREAD_INFO_NEXT_OFFSET               20
#define COTHREAD_INFO_FLAGS_WAIT_OFFSET         25
#define COTHREAD_INFO_WAIT_IRQ_MASK_OFFSET      32

#endif // LIBNDS_NDS_COTRHEAD_ASM_H__
