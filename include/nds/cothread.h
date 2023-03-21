// SPDX-License-Identifier: Zlib
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#ifndef NDS_COTHREAD_H__
#define NDS_COTHREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// A detached thread deallocates all memory used by it when it ends. Calling
// cothread_has_joined() or cothread_get_exit_code() isn't allowed.
#define COTHREAD_DETACHED   (1 << 0)

// Create a thread and allocate the stack for it. This stack will be freed when
// the thread is deleted.
//
// entrypoint: Function to be run. The first argument is an opaque pointer to a
//             context, unused for now. The second argument is the value of
//             'arg' passed to cothread_create().
// arg:        Argument to be passed to entrypoint.
// stack_size: Size of the stack. If it is set to zero it will use a default
//             value. If non-zero, it must be aligned to 32 bit.
// flags:      Set of ORed flags (for now, only COTHREAD_DETACHED) or 0.
//
// On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_create(int (*entrypoint)(void*, void*), void *arg,
                    size_t stack_size, unsigned int flags);

// Create a thread. The stack is owned by the caller of this function, and it
// has to be freed after the thread ends.
//
// entrypoint: Function to be run. The first argument is an opaque pointer to a
//             context, unused for now. The second argument is the value of
//             'arg' passed to cothread_create().
// arg:        Argument to be passed to entrypoint.
// stack_base: Pointer to the base of the memory to be used as stack. Must be
//             aligned to 32 bit.
// stack_size: Size of the stack. Must be aligned to 32 bit.
// flags:      Set of ORed flags (for now, only COTHREAD_DETACHED) or 0.
//
// On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_create_manual(int (*entrypoint)(void*, void*), void *arg,
                           void *stack_base, size_t stack_size,
                           unsigned int flags);

// Detach the specified thread.
//
// On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_detach(int thread);

// Used to determine if a thread is running or if it has ended (joined).
//
// Don't call this if the thread is detached.
//
// Returns true if the thread has ended, false otherwise. It can also set errno.
bool cothread_has_joined(int thread);

// If the thread has ended, this function returns the exit code.
//
// Don't call this if the thread is detached.
//
// Returns the exit code if the thread has finished, -1 otherwise. It will set
// errno as well (for example, if the thread is still running, it will set errno
// to EBUSY).
int cothread_get_exit_code(int thread);

// Deletes a running thread and frees all memory used by it. It isn't possible
// to delete the currently running thread.
//
// On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_delete(int thread);

// Tells the scheduler to switch to a different thread. This can also be called
// from main().
void cothread_sleep(void);

#ifdef __cplusplus
};
#endif

#endif // NDS_COTHREAD_H__
