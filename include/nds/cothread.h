// SPDX-License-Identifier: Zlib
//
// Copyright (c) 2023 Antonio Niño Díaz

#ifndef LIBNDS_NDS_COTHREAD_H__
#define LIBNDS_NDS_COTHREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/cothread.h
///
/// @brief Cooperative multithreading system
///
/// Only enabled in the ARM9 at the moment.

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/// Thread ID
typedef int cothread_t;
/// Mutex
typedef int comutex_t;

/// Flags a thread as detached.
///
/// A detached thread deallocates all memory used by it when it ends. Calling
/// cothread_has_joined() or cothread_get_exit_code() isn't allowed.
#define COTHREAD_DETACHED   (1 << 0)

/// Creates a thread and allocate the stack for it.
///
/// This stack will be freed when the thread is deleted.
///
/// @param entrypoint Function to be run. The argument is the value of 'arg'
///                   passed to cothread_create().
/// @param arg Argument to be passed to entrypoint.
/// @param stack_size Size of the stack. If it is set to zero it will use a
///                   default value. If non-zero, it must be aligned to 64 bit.
/// @param flags Set of ORed flags (like COTHREAD_DETACHED) or 0.
///
/// @return On success, it returns 0. On failure, it returns -1 and sets errno.
cothread_t cothread_create(int (*entrypoint)(void *), void *arg,
                           size_t stack_size, unsigned int flags);

/// Create a thread.
///
/// The stack is owned by the caller of this function, and it has to be freed
/// manually after the thread ends.
///
/// @param entrypoint Function to be run. The argument is the value of 'arg'
///                   passed to cothread_create_manual().
/// @param arg Argument to be passed to entrypoint.
/// @param stack_base Pointer to the base of the memory to be used as stack.
///                   It must be aligned to 64 bit.
/// @param stack_size Size of the stack. Must be aligned to 64 bit.
/// @param flags Set of ORed flags (like COTHREAD_DETACHED) or 0.
///
/// @return On success, it returns 0. On failure, it returns -1 and sets errno.
cothread_t cothread_create_manual(int (*entrypoint)(void *), void *arg,
                                  void *stack_base, size_t stack_size,
                                  unsigned int flags);

/// Detach the specified thread.
///
/// @param thread The thread to detach.
///
/// @return On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_detach(cothread_t thread);

/// Used to determine if a thread is running or if it has ended (joined).
///
/// Don't call this if the thread is detached. It will always return false
/// because as soon as the thread ends all information associated to it will be
/// deleted (and, at that point, it won't exist, so the function will return
/// false along an error code).
///
/// @param thread Thread ID.
///
/// @return Returns true if the thread has ended, false otherwise. It can also
/// set errno.
bool cothread_has_joined(cothread_t thread);

/// If the thread has ended, this function returns the exit code.
///
/// Don't call this if the thread is detached, it will never return an exit code
/// because the thread information will be deleted as soon as the thread ends
/// (and, at that point, it won't exist, so the function will return an error
/// code).
///
/// @param thread Thread ID.
///
/// @return Returns the exit code if the thread has finished, -1 otherwise. It
/// will set errno as well (for example, if the thread is still running, it
/// will set errno to EBUSY).
int cothread_get_exit_code(cothread_t thread);

/// Deletes a running thread and frees all memory used by it.
///
/// It isn't possible to delete the currently running thread.
///
/// @param thread Thread ID.
///
/// @return On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_delete(cothread_t thread);

/// Tells the scheduler to switch to a different thread.
///
/// This can also be called from main().
void cothread_yield(void);

/// Tells the scheduler to switch to a different thread until the specified IRQ
/// has happened.
///
/// @param flags IRQ flags to wait for.
void cothread_yield_irq(uint32_t flags);

#ifdef ARM7
/// Tells the scheduler to switch to a different thread until the specified ARM7
/// AUX IRQ has happened.
///
/// @param flags AUX IRQ flags to wait for.
/// @note ARM7 only.
void cothread_yield_irq_aux(uint32_t flags);
#endif

/// Returns ID of the thread that is running currently.
///
/// @return Thread ID of the current thread.
cothread_t cothread_get_current(void);

/// Initializes a mutex.
///
/// @param mutex Pointer to the mutex.
///
/// @return It returns true if the mutex has been initialized, false if not.
static inline bool comutex_init(comutex_t *mutex)
{
    *mutex = 0;
    return true;
}

/// Tries to acquire a mutex without blocking execution.
///
/// @param mutex Pointer to the mutex.
///
/// @return It returns true if the mutex has been acquired, false if not.
static inline bool comutex_try_acquire(comutex_t *mutex)
{
    if (*mutex != 0)
        return false;

    *mutex = 1;
    return true;
}

/// Waits in a loop until the mutex is available.
///
/// The main body of the loop calls cothread_yield() after each try, so that
/// other threads can take control of the CPU and eventually release the mutex.
///
/// @param mutex Pointer to the mutex.
static inline void comutex_acquire(comutex_t *mutex)
{
    while (comutex_try_acquire(mutex) == false)
        cothread_yield();
}

/// Releases a mutex.
///
/// @param mutex Pointer to the mutex.
static inline void comutex_release(comutex_t *mutex)
{
    *mutex = 0;
}

// Private thread information. It is private to the library, but exposed here
// to make it possible to write tests for cothread. It extends __ndsabi_coro_t.
typedef struct
{
    // Present in __ndsabi_coro_t
    uint32_t arm_sp : 31;
    uint32_t joined : 1;
    uint32_t arg;

    // Specific to cothread
    void *stack_base; // If not NULL, it has to be freed by the scheduler
    void *tls;
    void *next;
    uint32_t wait_irq_flags;
#ifdef ARM7
    uint32_t wait_irq_aux_flags;
#endif
    uint32_t flags;
} cothread_info_t;

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_COTHREAD_H__
