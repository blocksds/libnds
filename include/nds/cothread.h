// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2025 Antonio Niño Díaz

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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <nds/cothread_asm.h>

/// Thread ID
typedef int cothread_t;
/// Mutex
typedef volatile uint8_t comutex_t;
/// Semaphore (counting, not binary)
typedef volatile uint32_t cosema_t;
/// Thread entrypoint
typedef int (*cothread_entrypoint_t)(void *);

/// Creates a thread and allocate the stack for it.
///
/// This stack will be freed when the thread is deleted.
///
/// Important: If this thread is going to do filesystem accesses, you need to
/// assign it a reasonably big stack size.
///
/// @param entrypoint
///     Function to be run. The argument is the value of 'arg' passed to
///     cothread_create().
/// @param arg
///     Argument to be passed to entrypoint.
/// @param stack_size
///     Size of the stack. If it is set to zero it will use a default value. If
///     non-zero, it must be aligned to 64 bit.
/// @param flags
///     Set of ORed flags (like COTHREAD_DETACHED) or 0.
///
/// @return
///     On success, it returns a non-negative value representing the thread ID.
///     On failure, it returns -1 and sets errno.
cothread_t cothread_create(cothread_entrypoint_t entrypoint, void *arg,
                           size_t stack_size, unsigned int flags);

/// Create a thread.
///
/// The stack is owned by the caller of this function, and it has to be freed
/// manually after the thread ends.
///
/// @param entrypoint
///     Function to be run. The argument is the value of 'arg' passed to
///     cothread_create_manual().
/// @param arg
///     Argument to be passed to entrypoint.
/// @param stack_base
///     Pointer to the base of the memory to be used as stack.  It must be
///     aligned to 64 bit.
/// @param stack_size
///     Size of the stack. Must be aligned to 64 bit.
/// @param flags
///     Set of ORed flags (like COTHREAD_DETACHED) or 0.
///
/// @return
///     On success, it returns a non-negative value representing the thread ID.
///     On failure, it returns -1 and sets errno.
cothread_t cothread_create_manual(cothread_entrypoint_t entrypoint, void *arg,
                                  void *stack_base, size_t stack_size,
                                  unsigned int flags);

/// Detach the specified thread.
///
/// @param thread
///     The thread to detach.
///
/// @return
///     On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_detach(cothread_t thread);

/// Used to determine if a thread is running or if it has ended (joined).
///
/// Don't call this if the thread is detached. It will always return false
/// because as soon as the thread ends all information associated to it will be
/// deleted (and, at that point, it won't exist, so the function will return
/// false along an error code).
///
/// @param thread
///     Thread ID.
///
/// @return
///     Returns true if the thread has ended, false otherwise. It can also set
///     errno.
bool cothread_has_joined(cothread_t thread);

/// If the thread has ended, this function returns the exit code.
///
/// Don't call this if the thread is detached, it will never return an exit code
/// because the thread information will be deleted as soon as the thread ends
/// (and, at that point, it won't exist, so the function will return an error
/// code).
///
/// @param thread
///     Thread ID.
///
/// @return
///     Returns the exit code if the thread has finished, -1 otherwise. It will
///     set errno as well (for example, if the thread is still running, it will
///     set errno to EBUSY).
int cothread_get_exit_code(cothread_t thread);

/// Deletes a running thread and frees all memory used by it.
///
/// It isn't possible to delete the currently running thread.
///
/// @param thread
///     Thread ID.
///
/// @return
///     On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_delete(cothread_t thread);

/// Tells the scheduler to switch to a different thread.
///
/// This can also be called from main().
void cothread_yield(void);

/// Tells the scheduler to switch to a different thread until the specified IRQ
/// has happened.
///
/// @param flag
///     IRQ flag to wait for (only one).
void cothread_yield_irq(uint32_t flag);

#ifdef ARM7
/// Tells the scheduler to switch to a different thread until the specified ARM7
/// AUX IRQ has happened.
///
/// @param flag
///     AUX IRQ flag to wait for (only one).
///
/// @note
///     ARM7 only.
void cothread_yield_irq_aux(uint32_t flag);
#endif

/// Tells the scheduler to switch to a different thread until the specified
/// signal ID is received.
///
/// The thread will wait until cothread_send_signal() is called with the same
/// signal ID.
///
/// User-defined signal IDs aren't allowed to use numbers greater than
/// 0x7FFFFFFF. Bit 31 is reserved for system signal IDs.
///
/// @param signal_id
///     A user-defined number.
void cothread_yield_signal(uint32_t signal_id);

/// Awake threads waiting for the provided signal ID.
///
/// All threads waiting for this signal ID will wake up.
///
/// User-defined signal IDs aren't allowed to use numbers greater than
/// 0x7FFFFFFF. Bit 31 is reserved for system signal IDs.
///
/// @param signal_id
///     A user-defined number.
void cothread_send_signal(uint32_t signal_id);

/// Returns ID of the thread that is running currently.
///
/// @return
///     Thread ID of the current thread.
cothread_t cothread_get_current(void);

/// Initializes a mutex.
///
/// @param mutex
///     Pointer to the mutex.
///
/// @return
///     It returns true if the mutex has been initialized, false if not.
static inline bool comutex_init(comutex_t *mutex)
{
    *mutex = 0;
    return true;
}

/// Tries to acquire a mutex without blocking execution.
///
/// @param mutex
///     Pointer to the mutex.
///
/// @return
///     It returns true if the mutex has been acquired, false if not.
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
/// @param mutex
///     Pointer to the mutex.
static inline void comutex_acquire(comutex_t *mutex)
{
    while (comutex_try_acquire(mutex) == false)
        cothread_yield();
}

/// Releases a mutex.
///
/// @param mutex
///     Pointer to the mutex.
static inline void comutex_release(comutex_t *mutex)
{
    *mutex = 0;
}

/// Initializes a counting semaphore to the desired value.
///
/// @param sema
///     Pointer to the semaphore.
/// @param init_val
///     Initial value (zero or a positive integer).
///
/// @return
///     It returns true if the semaphore has been initialized, false if not.
static inline bool cosema_init(cosema_t *sema, uint32_t init_val)
{
    *sema = init_val;
    return true;
}

/// Signals a semaphore.
///
/// It increases the semaphore counter so that other threads can access the
/// resources protected by the semaphore.
///
/// @param sema
///     Pointer to the semaphore.
static inline void cosema_signal(cosema_t *sema)
{
    *sema = *sema + 1;
}

/// Checks if a semaphore has been signalled.
///
/// It checks the value of the semaphore and returns right away instead of
/// waiting for the semaphore to be signalled.
///
/// @param sema
///     Pointer to the semaphore.
///
/// @return
///     If the semaphore has been signalled it returns true. If not, false.
static inline bool cosema_try_wait(cosema_t *sema)
{
    if (*sema > 0)
    {
        *sema = *sema - 1;
        return true;
    }

    return false;
}

/// Waits in a loop until the semaphore is signalled.
///
/// The main body of the loop calls cothread_yield() after each try, so that
/// other threads can take control of the CPU and eventually signal the
/// semaphore.
///
/// @param sema
///     Pointer to the semaphore.
static inline void cosema_wait(cosema_t *sema)
{
    while (cosema_try_wait(sema) == false)
        cothread_yield();
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
    void *next; // Next thread in the global list of threads
    union {
        void *next_irq; // Next thread in the list of threads waiting for the same IRQ
        void *next_signal; // Next thread in the list of threads waiting for a signal
    };
    union {
        uint32_t wait_signal_id; // Signal ID the thread is waiting for
    };
    uint32_t flags; // COTHREAD_DETACHED, COTHREAD_WAIT_IRQ, etc
} cothread_info_t;

static_assert(offsetof(cothread_info_t, next_irq) == COTHREAD_INFO_NEXT_IRQ_OFFSET);
static_assert(offsetof(cothread_info_t, flags) == COTHREAD_INFO_FLAGS_OFFSET);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_COTHREAD_H__
