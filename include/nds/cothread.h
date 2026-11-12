// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#ifndef LIBNDS_NDS_COTHREAD_H__
#define LIBNDS_NDS_COTHREAD_H__

#ifdef __cplusplus
extern "C" {
#endif

/// @file nds/cothread.h
///
/// @brief Multithreading system
///
/// This system allows cooperative or preemptive multithreading. By default,
/// it's cooperative, and it can be switched to preemptive by calling
/// `cothread_preemption_enable()`.
///
/// It's only enabled in the ARM9 at the moment.

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <nds/ndstypes.h>
#include <nds/cothread_asm.h>

/// Thread ID
typedef int cothread_t;
/// Mutex
typedef volatile uint8_t comutex_t;
/// Semaphore (counting, not binary)
typedef volatile uint32_t cosema_t;
/// Thread entrypoint
typedef int (*cothread_entrypoint_t)(void *);

// Internal helper to create a signal ID from a comutex_t
static inline uint32_t comutex_to_signal_id(comutex_t *mutex)
{
    return BIT(31) | (uintptr_t)mutex;
}

// Internal helper to create a signal ID from a cosema_t
static inline uint32_t cosema_to_signal_id(cosema_t *sema)
{
    return BIT(31) | (uintptr_t)sema;
}

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
///     ORed option flags (COTHREAD_DETACHED, COTHREAD_CONTEXT_IN_DTCM...).
///
/// @return
///     On success, it returns a non-negative value representing the thread ID.
///     On failure, it returns -1 and sets errno.
cothread_t cothread_create(cothread_entrypoint_t entrypoint, void *arg,
                           size_t stack_size, uint16_t flags);

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
///     ORed option flags (COTHREAD_DETACHED, COTHREAD_CONTEXT_IN_DTCM...).
///
/// @return
///     On success, it returns a non-negative value representing the thread ID.
///     On failure, it returns -1 and sets errno.
cothread_t cothread_create_manual(cothread_entrypoint_t entrypoint, void *arg,
                                  void *stack_base, size_t stack_size,
                                  uint16_t flags);

/// Enables or disables preemption in the multithreading system.
///
/// When preemption is disabled, the only way to switch between thread contexts
/// is to call the `cothread_yield()` functions. When it is enabled, even if you
/// don't call the yield functions, whenever an interrupt is handled, there will
/// be a context switch. This allows you to have a high priority thread that
/// runs as soon as a specific IRQ happens, for example.
///
/// @param enabled
///     If true, it will enable preemption. If false, it will disable it.
void cothread_preemption_enable(bool enabled);

/// Gets the priority of the specified thread.
///
/// @param thread
///     The thread to get the priority of. If this is 0, it will return the
///     priority of the current active thread.
///
/// @return
///     On success, it returns the priority of the thread. On failure, it
///     returns -128 (the minimum possible priority) and it sets errno.
int8_t cothread_get_priority(cothread_t thread);

/// Sets the priority of the specified thread.
///
/// Threads inherit the priority of the thread that created them. The main()
/// thread starts with priority 0 (base priority).
///
/// @warning
///     If a thread with high priority never yields for IRQs or events, it will
///     starve lower priority threads.
///
/// @param thread
///     The thread to modify. If this is 0, it will change the priority of the
///     current active thread.
/// @param priority
///     New priority. The maximum (highest priority) is 127, the minimum (lowest
///     priority) is -128.
///
/// @return
///     On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_set_priority(cothread_t thread, int8_t priority);

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
/// @param mask
///     Mask of IRQ flags to wait for (IRQ_VBLANK, IRQ_VCOUNT, IRQ_ALL, etc).
void cothread_yield_irq(uint32_t mask);

#ifdef ARM7
/// Tells the scheduler to switch to a different thread until the specified ARM7
/// AUX IRQ has happened.
///
/// @param flag
///     Mask of AUX IRQ flags to wait for.
///
/// @note
///     ARM7 only.
void cothread_yield_irq_aux(uint32_t mask);
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

/// Tells the scheduler to switch to a different thread until the specified IRQ
/// or the specified event have happened.
///
/// @param mask
///     Mask of IRQ flags to wait for (IRQ_VBLANK, IRQ_VCOUNT, IRQ_ALL, etc).
/// @param signal_id
///     A user-defined number (zero isn't a valid ID).
void cothread_yield_irq_or_signal(uint32_t mask, uint32_t signal_id);

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
bool comutex_init(comutex_t *mutex);

/// Tries to acquire a mutex without blocking execution.
///
/// @param mutex
///     Pointer to the mutex.
///
/// @return
///     It returns true if the mutex has been acquired, false if not.
bool comutex_try_acquire(comutex_t *mutex);

/// Waits in a loop until the mutex is available.
///
/// The main body of the loop yields after each try so that other threads can
/// take control of the CPU and eventually release the mutex.
///
/// @param mutex
///     Pointer to the mutex.
void comutex_acquire(comutex_t *mutex);

/// Releases a mutex.
///
/// It also sends a signal to all threads that may be waiting for this mutex.
///
/// @param mutex
///     Pointer to the mutex.
void comutex_release(comutex_t *mutex);

/// Initializes a counting semaphore to the desired value.
///
/// @param sema
///     Pointer to the semaphore.
/// @param init_val
///     Initial value (zero or a positive integer).
///
/// @return
///     It returns true if the semaphore has been initialized, false if not.
bool cosema_init(cosema_t *sema, uint32_t init_val);

/// Signals a semaphore.
///
/// It increases the semaphore counter so that other threads can access the
/// resources protected by the semaphore. It also sends a signal to all threads
/// that may be waiting for this semaphore.
///
/// @param sema
///     Pointer to the semaphore.
void cosema_signal(cosema_t *sema);

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
bool cosema_try_wait(cosema_t *sema);

/// Waits in a loop until the semaphore is signalled.
///
/// The main body of the loop yields after each try so that other threads can
/// take control of the CPU and eventually signal the semaphore.
///
/// @param sema
///     Pointer to the semaphore.
void cosema_wait(cosema_t *sema);

// Private thread information. It is private to the library, but exposed here
// to make it possible to write tests for cothread.
typedef struct
{
    // Context of the thread while it's sleeping
    struct
    {
        uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
        uint32_t sp, lr, pc;
        uint32_t cpsr;
    }
    cpu;

    // Value returned by a thread. Detached threads never give the user the
    // possibility to read it because the context is freed right after it ends.
    uint32_t exit_code;

    void *stack_base;
    void *stack_top;
    void *tls;

    // Previous and next thread in the linked list the thread belogs to
    void *next;
    void *prev;

    // Flags like COTHREAD_DETACHED, COTHREAD_ALLOCATED_CONTEXT...
    uint16_t flags;

    // Thread priority. Maximum priority is 127, base priority is 0, minimum
    // priority is -128.
    int8_t priority;

    // Current thread state: COTHREAD_STATE_ACTIVE, COTHREAD_STATE_WAITING...
    uint8_t state;

    uint32_t wait_signal_id; // Signal ID the thread is waiting for (0 = no signal)
    uint32_t wait_irq_mask; // Mask of IRQ bits the thread is waiting for
#ifdef ARM7
    uint32_t wait_irq_aux_mask; // Extended ARM7 interrupts
#endif
}
cothread_info_t;

static_assert(offsetof(cothread_info_t, cpu) == COTHREAD_INFO_CPU_OFFSET);
static_assert(offsetof(cothread_info_t, cpu.r4) == COTHREAD_INFO_CPU_R4_OFFSET);
static_assert(offsetof(cothread_info_t, cpu.r12) == COTHREAD_INFO_CPU_R12_OFFSET);
static_assert(offsetof(cothread_info_t, cpu.sp) == COTHREAD_INFO_CPU_SP_OFFSET);
static_assert(offsetof(cothread_info_t, cpu.lr) == COTHREAD_INFO_CPU_LR_OFFSET);
static_assert(offsetof(cothread_info_t, cpu.pc) == COTHREAD_INFO_CPU_PC_OFFSET);
static_assert(offsetof(cothread_info_t, cpu.cpsr) == COTHREAD_INFO_CPU_CPSR_OFFSET);
static_assert(offsetof(cothread_info_t, exit_code) == COTHREAD_INFO_EXIT_CODE_OFFSET);
static_assert(offsetof(cothread_info_t, state) == COTHREAD_INFO_STATE_OFFSET);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_COTHREAD_H__
