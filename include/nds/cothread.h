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

typedef int cothread_t;
typedef int comutex_t;

// A detached thread deallocates all memory used by it when it ends. Calling
// cothread_has_joined() or cothread_get_exit_code() isn't allowed.
#define COTHREAD_DETACHED   (1 << 0)

// Create a thread and allocate the stack for it. This stack will be freed when
// the thread is deleted.
//
// entrypoint: Function to be run. The argument is the value of 'arg' passed to
//             cothread_create().
// arg:        Argument to be passed to entrypoint.
// stack_size: Size of the stack. If it is set to zero it will use a default
//             value. If non-zero, it must be aligned to 32 bit.
// flags:      Set of ORed flags (for now, only COTHREAD_DETACHED) or 0.
//
// On success, it returns 0. On failure, it returns -1 and sets errno.
cothread_t cothread_create(int (*entrypoint)(void *), void *arg,
                           size_t stack_size, unsigned int flags);

// Create a thread. The stack is owned by the caller of this function, and it
// has to be freed after the thread ends.
//
// entrypoint: Function to be run. The argument is the value of 'arg' passed to
//             cothread_create_manual().
// arg:        Argument to be passed to entrypoint.
// stack_base: Pointer to the base of the memory to be used as stack. Must be
//             aligned to 32 bit.
// stack_size: Size of the stack. Must be aligned to 32 bit.
// flags:      Set of ORed flags (for now, only COTHREAD_DETACHED) or 0.
//
// On success, it returns 0. On failure, it returns -1 and sets errno.
cothread_t cothread_create_manual(int (*entrypoint)(void *), void *arg,
                                  void *stack_base, size_t stack_size,
                                  unsigned int flags);

// Detach the specified thread.
//
// On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_detach(cothread_t thread);

// Used to determine if a thread is running or if it has ended (joined).
//
// Don't call this if the thread is detached. It will always return false
// because as soon as the thread ends all information associated to it will be
// deleted (and, at that point, it won't exist, so the function will return
// false along an error code).
//
// Returns true if the thread has ended, false otherwise. It can also set errno.
bool cothread_has_joined(cothread_t thread);

// If the thread has ended, this function returns the exit code.
//
// Don't call this if the thread is detached, it will never return an exit code
// because the thread information will be deleted as soon as the thread ends
// (and, at that point, it won't exist, so the function will return an error
// code).
//
// Returns the exit code if the thread has finished, -1 otherwise. It will set
// errno as well (for example, if the thread is still running, it will set errno
// to EBUSY).
int cothread_get_exit_code(cothread_t thread);

// Deletes a running thread and frees all memory used by it. It isn't possible
// to delete the currently running thread.
//
// On success, it returns 0. On failure, it returns -1 and sets errno.
int cothread_delete(cothread_t thread);

// Tells the scheduler to switch to a different thread. This can also be called
// from main().
void cothread_yield(void);

// Tells the scheduler to switch to a different thread until the specified IRQ
// has happened.
void cothread_yield_irq(uint32_t flags);

#ifdef ARM7
// Tells the scheduler to switch to a different thread until the specified ARM7
// AUX IRQ has happened.
void cothread_yield_irq_aux(uint32_t flags);
#endif

// Returns ID of the thread that is running currently.
cothread_t cothread_get_current(void);

// Try to acquire a mutex. If the mutex is available, it is acquired and the
// function returns true. If the mutex can't be acquired, it returns false.
bool comutex_try_acquire(comutex_t *mutex);

// Waits in a loop until the mutex is available. The main body of the loop calls
// cothread_yield() after each try, so that other threads can take control of
// the CPU and eventually release the mutex.
void comutex_acquire(comutex_t *mutex);

// Releases a mutex.
void comutex_release(comutex_t *mutex);

#ifdef __cplusplus
};
#endif

#endif // NDS_COTHREAD_H__
