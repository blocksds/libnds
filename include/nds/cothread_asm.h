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
#define COTHREAD_DETACHED           (1 << 0)

#ifdef ARM9

/// Makes cothread_create() or cothread_create_manual() allocate the thread
/// context in ITCM.
#define COTHREAD_CONTEXT_IN_ITCM    (1 << 1)

/// Makes cothread_create() or cothread_create_manual() allocate the thread
/// context in DTCM.
#define COTHREAD_CONTEXT_IN_DTCM    (1 << 2)

/// Makes cothread_create() allocate the thread stack in ITCM.
#define COTHREAD_STACK_IN_ITCM      (1 << 3)

/// Makes cothread_create() allocate the thread stack in DTCM.
#define COTHREAD_STACK_IN_DTCM      (1 << 4)

#endif // ARM9

// Resources flags (internal)
// --------------------------

// The following flags are used to remember which resources have been allocated
// by the cothread code. If they aren't set, the user has allocated them.
#define COTHREAD_ALLOCATED_CONTEXT (1 << 14)
#define COTHREAD_ALLOCATED_STACK   (1 << 15)

// Thread state
// ------------

#define COTHREAD_STATE_INVALID      0
#define COTHREAD_STATE_ACTIVE       1
#define COTHREAD_STATE_WAITING      2
#define COTHREAD_STATE_ENDED        3

// cothread_info_t
// ---------------

// Offsets to fields inside the cothread_info_t struct
#define COTHREAD_INFO_STACK_POINTER_OFFSET      (13 * 4)
#define COTHREAD_INFO_EXIT_CODE_OFFSET          (17 * 4)
#define COTHREAD_INFO_STATE_OFFSET              (27 + 17 * 4)

#endif // LIBNDS_NDS_COTRHEAD_ASM_H__
