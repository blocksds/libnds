// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2025 Antonio Niño Díaz

#include <assert.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>

#include <ndsabi.h>
#ifdef ARM9
#include <nds/arm9/cp15.h>
#endif
#include <nds/bios.h>
#include <nds/cothread.h>
#include <nds/exceptions.h>
#include <nds/interrupts.h>
#include <nds/ndstypes.h>

// Generate a reference to __retarget_lock_acquire(). This will force the linker
// to add the version of the function included in libnds.
//
// picolibc has a placeholder implementation of the __retarget_lock family of
// functions. libnds has the actual implementation we need. This family of
// functions is used in multithreaded environments by some libc functions
// (stdio, malloc, etc).
//
// If we add this reference to libnds, the linker will take the functions from
// libnds, even if they aren't directly used by libnds. Then, when picolibc is
// linked, it won't try to find them again as they are already found. Note that
// it's only required to add a reference to one of the symbols for the rest of
// the symbols to be added correctly.
//
// If we don't add this reference to libnds, the linker will realize that
// nothing in libnds uses the functions directly, and it will remove them. Then,
// when picolibc is linked, it will detect that the functions are actually
// called, but it will take them from picolibc (the placeholder version).
//
// For this reference trick to work, the file needs to always be linked in if
// the library is used. Currently, cothread_start() is called from the crt0, and
// the cothread functions are the ones that require the __retarget_lock
// functions. By leaving this reference here, we only add the functions to
// binaries that call cothread_start(). Currently, only the ARM9 has
// multithreading enabled, while the ARM7 doesn't. This means that only the ARM9
// binary will have the functions, not the ARM7, even if this file is common to
// both CPUs.
__asm__(".equ __retarget_lock_acquire_reference, __retarget_lock_acquire");

#define DEFAULT_STACK_SIZE_CHILD (1 * 1024)

// This is a trick so that the garbage collector of the linker can remove free()
// from any application that doesn't actually create any thread. This pointer is
// set to free() when any thread is created. At that point, free is needed to
// clean the resources used by the newly created thread. The main() thread is
// never freed, so this is only needed when a second thread is created.
static void (*free_fn)(void *) = NULL;

// Thread that is currently running
static cothread_info_t *cothread_active_thread = NULL;

// This context is the start of the linked list that contains the contexts of
// all threads. It is also used for the main() thread, which can never be freed.
static cothread_info_t cothread_list;
// TODO: When a non-detached thread ends, remove it from this list and add it to
// a list of finished-but-not-deleted threads.

// 32 list of threads waiting for interrupts for the 32 bits in the registers IE
// and IF.
ITCM_BSS cothread_info_t *cothread_list_irq[32];
#ifdef ARM7
cothread_info_t *cothread_list_irq_aux[32];
#endif

// List of threads waiting for signals.
ITCM_BSS cothread_info_t *cothread_list_signal;

// Total number of threads
ITCM_BSS uint32_t cothread_threads_count;

// Total number of threads waiting for events such as interrupts
ITCM_BSS uint32_t cothread_threads_waiting_count;

//-------------------------------------------------------------------

// Linker symbols
extern char __tdata_start[];
extern char __tdata_size[];

extern char __tbss_start[];
extern char __tbss_size[];

extern char __tls_start[];
extern char __tls_end[];

void init_tls(void *__tls)
{
    char *tls = __tls;

    char *tdata_start = tls;
    // The linker places tbss right after tdata
    char *tbss_start = tls + (uintptr_t)__tdata_size;

    // Copy tdata
    memcpy(tdata_start, __tdata_start, (uintptr_t)__tdata_size);

    // Clear tbss
    memset(tbss_start, 0, (uintptr_t)__tbss_size);
}

// Size of a thread control block. TLS relocations are generated relative to a
// location before tdata and tbss.
#define TCB_SIZE 8

// This holds the pointer to the TLS of the current thread for __aeabi_read_tp.
// It doesn't hold the pointer to the start of the TLS data, but to to the
// beginning of the thread control block.
//
// On the ARM9 it's placed in ITCM because it's closer to the code accessing it.
// Also, placing it in DTCM would force users to hardcode the size of DTCM in
// the linker (by setting __dtcm_data_size). If not, this variable would be
// placed at the start of DTCM, so the stack wouldn't be able to grow to main
// RAM if it runs out of space in DTCM.
ITCM_DATA void *__tls = __tls_start - TCB_SIZE;

static inline void set_tls(void *tls)
{
    __tls = (uint8_t *)tls - TCB_SIZE;
}

//-------------------------------------------------------------------

static void cothread_list_add_ctx(cothread_info_t *ctx)
{
    // Find last node of the list

    cothread_info_t *p = &cothread_list;

    while (p->next != NULL)
        p = p->next;

    // Append the new context to the end

    p->next = ctx;
}

ITCM_CODE static void cothread_list_remove_ctx_from_irq_list(cothread_info_t *ctx)
{
    // Look for this thread context in all the list of interrupts and remove it
    // from there.

    for (int i = 0; i < 32; i++)
    {
        int oldIME = enterCriticalSection();

        cothread_info_t **list = &cothread_list_irq[i];

        while (*list != NULL)
        {
            if (*list == ctx)
            {
                // Remove from list
                *list = (*list)->next_irq;
                cothread_threads_waiting_count--;
                leaveCriticalSection(oldIME);
                return;
            }

            list = (cothread_info_t **)&((*list)->next_irq);
        }

#ifdef ARM7
        list = &cothread_list_irq_aux[i];

        while (*list != NULL)
        {
            if (*list == ctx)
            {
                // Remove from list
                *list = (*list)->next_irq;
                cothread_threads_waiting_count--;
                leaveCriticalSection(oldIME);
                return;
            }

            list = (cothread_info_t **)&((*list)->next_irq);
        }
#endif
        leaveCriticalSection(oldIME);
    }
}

ITCM_CODE static void cothread_list_remove_ctx(cothread_info_t *ctx)
{
    // Remove context from lists of interrupts
    cothread_list_remove_ctx_from_irq_list(ctx);

    // Now, remove the context from the global list of threads. The first
    // element of cothread_list is statically allocated. It is the main()
    // thread, which can never be deleted.

    cothread_info_t *p = &cothread_list;

    for (; p->next != NULL; p = p->next)
    {
        if (p->next == ctx)
        {
            // Skip the context that we have just found
            p->next = ((cothread_info_t *)p->next)->next;
            return;
        }
    }

    // Reaching this point means that there is a bug somewhere in the code.
    libndsCrash(__func__);
}

static bool cothread_list_contains_ctx(cothread_info_t *ctx)
{
    cothread_info_t *p = &cothread_list;

    while (1)
    {
        if (p == ctx)
            return true;

        if (p->next == NULL)
            return false;

        p = p->next;
    }
}

//-------------------------------------------------------------------

static void cothread_delete_internal(cothread_info_t *ctx)
{
    cothread_list_remove_ctx(ctx);

    if (ctx->stack_base)
        free_fn(ctx->stack_base);

    free(ctx->tls);

    free_fn(ctx);

    cothread_threads_count--;
}

int cothread_delete(cothread_t thread)
{
    cothread_info_t *ctx = (cothread_info_t *)thread;

    if (ctx == cothread_active_thread)
    {
        errno = EPERM;
        return -1;
    }

    if (!cothread_list_contains_ctx(ctx))
    {
        errno = EINVAL;
        return -1;
    }

    cothread_delete_internal(ctx);

    return 0;
}

static cothread_t cothread_create_internal(cothread_info_t *ctx,
                                           int (*entrypoint)(void *), void *arg,
                                           void *stack_top, void *tls,
                                           unsigned int flags)
{
    ctx->flags = flags;
    ctx->tls = tls;

    // Initialize context
    __ndsabi_coro_make_noctx((void *)ctx, stack_top, entrypoint, arg);

    cothread_threads_count++;

    return (cothread_t)ctx;
}

cothread_t cothread_create_manual(int (*entrypoint)(void *), void *arg,
                                  void *stack_base, size_t stack_size,
                                  unsigned int flags)
{
    // stack_size can be zero, like for the main() thread.

    if ((stack_base == NULL) || (entrypoint == NULL))
        goto invalid_args;

    // They must be aligned to 8 bytes
    if (((stack_size & 7) != 0) || (((uintptr_t)stack_base & 7) != 0))
        goto invalid_args;

    // Setup context

    cothread_info_t *ctx = calloc(1, sizeof(cothread_info_t));
    if (ctx == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    size_t __tls_size = (uintptr_t)__tls_end - (uintptr_t)__tls_start;

    void *tls = malloc(__tls_size);
    if (tls == NULL)
    {
        free(ctx);
        errno = ENOMEM;
        return -1;
    }

    init_tls(tls);

    // Assign the free() function to the pointer because now we are sure that we
    // will need to free the resources of the newly created thread eventually.

    free_fn = free;

    // Add context to the scheduler
    cothread_list_add_ctx(ctx);

    void *stack_top = (void *)((uintptr_t)stack_base + stack_size);

    return cothread_create_internal(ctx, entrypoint, arg, stack_top, tls, flags);

invalid_args:
    errno = EINVAL;
    return -1;
}

cothread_t cothread_create(int (*entrypoint)(void *), void *arg,
                           size_t stack_size, unsigned int flags)
{
    // Setup stack

    if ((stack_size & 7) != 0)
    {
        errno = EINVAL;
        return -1;
    }

    if (stack_size == 0)
        stack_size = DEFAULT_STACK_SIZE_CHILD;

    // The stack must be aligned to 8 bytes
    void *stack_base = memalign(8, stack_size);
    if (stack_base == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    // Create thread

    cothread_t id = cothread_create_manual(entrypoint, arg,
                                           stack_base, stack_size, flags);
    if (id == -1)
    {
        free_fn(stack_base);
        return -1;
    }

    // Set this stack pointer as owned by cothread

    cothread_info_t *ctx = (cothread_info_t *)id;
    ctx->stack_base = stack_base;

    return id;
}

int cothread_detach(cothread_t thread)
{
    cothread_info_t *ctx = (cothread_info_t *)thread;

    if (!cothread_list_contains_ctx(ctx))
    {
        errno = EINVAL;
        return -1;
    }

    ctx->flags |= COTHREAD_DETACHED;

    return 0;
}

bool cothread_has_joined(cothread_t thread)
{
    cothread_info_t *ctx = (cothread_info_t *)thread;

    if (!cothread_list_contains_ctx(ctx))
    {
        errno = EINVAL;
        return false;
    }

    return ctx->joined != 0;
}

int cothread_get_exit_code(cothread_t thread)
{
    cothread_info_t *ctx = (cothread_info_t *)thread;

    if (!cothread_list_contains_ctx(ctx))
    {
        errno = EINVAL;
        return -1;
    }

    if (ctx->joined == 0)
    {
        errno = EBUSY;
        return -1;
    }

    return ctx->arg;
}

extern uint16_t irq_nesting_level;

void cothread_yield(void)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
        return;

    cothread_info_t *ctx = cothread_active_thread;

    __ndsabi_coro_yield((void *)ctx, 0);
}

ARM_CODE void cothread_yield_irq(uint32_t flag)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
    {
        if (REG_IME == 1)
            swiIntrWait(INTRWAIT_KEEP_FLAGS, flag);

        return;
    }

    assert(__builtin_popcount(flag) == 1); // There must be one bit set exactly

    cothread_info_t *ctx = cothread_active_thread;

    unsigned int index = __builtin_ctz(flag);

    REG_IME = 0;

    if (cothread_list_irq[index] != NULL)
    {
        ctx->next_irq = cothread_list_irq[index];
        cothread_list_irq[index] = ctx;
    }
    else
    {
        cothread_list_irq[index] = ctx;
    }

    ctx->flags |= COTHREAD_WAITING;

    // It isn't needed to check if the interrupt is in the list twice. This
    // should never happen because a thread waiting for an interrupt should
    // never be scheduled again until that interrupt has happened.

    cothread_threads_waiting_count++;

    // We're going to wait for an IRQ. Make sure that IRQs are enabled.
    REG_IME = 1;

    __ndsabi_coro_yield((void *)ctx, 0);
}

#ifdef ARM7
ARM_CODE void cothread_yield_irq_aux(uint32_t flag)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
    {
        if (REG_IME == 1)
            swiIntrWaitAUX(INTRWAIT_KEEP_FLAGS, 0, flag);

        return;
    }

    assert(__builtin_popcount(flag) == 1); // There must be one bit set exactly

    cothread_info_t *ctx = cothread_active_thread;

    unsigned int index = __builtin_ctz(flag);

    REG_IME = 0;

    if (cothread_list_irq_aux[index] != NULL)
    {
        ctx->next_irq = cothread_list_irq_aux[index];
        cothread_list_irq_aux[index] = ctx;
    }
    else
    {
        cothread_list_irq_aux[index] = ctx;
    }

    ctx->flags |= COTHREAD_WAITING;

    cothread_threads_waiting_count++;

    // We're going to wait for an IRQ. Make sure that IRQs are enabled.
    REG_IME = 1;

    __ndsabi_coro_yield((void *)ctx, 0);
}
#endif

ITCM_CODE void cothread_yield_signal(uint32_t signal_id)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
        return;

    cothread_info_t *ctx = cothread_active_thread;

    if (cothread_list_signal != NULL)
    {
        ctx->next_signal = cothread_list_signal;
        cothread_list_signal = ctx;
    }
    else
    {
        cothread_list_signal = ctx;
    }

    ctx->wait_signal_id = signal_id;
    ctx->flags |= COTHREAD_WAITING;

    cothread_threads_waiting_count++;

    __ndsabi_coro_yield((void *)ctx, 0);
}

ITCM_CODE void cothread_send_signal(uint32_t signal_id)
{
    int count = 0;

    cothread_info_t *ctx_prev = NULL;
    cothread_info_t *ctx = cothread_list_signal;

    while (ctx != NULL)
    {
        // Skip threads waiting for a different signal ID
        if (ctx->wait_signal_id != signal_id)
        {
            ctx_prev = ctx;
            ctx = ctx->next_signal;
            continue;
        }

        // If this thread is waiting for the signal ID, remove the "waiting"
        // flag and remove it from the cothread_list_signal list.

        ctx->flags &= ~COTHREAD_WAITING;

        if (ctx_prev == NULL)
        {
            // If this is the first element in the list, make
            // cothread_list_signal point to the new first element.
            cothread_list_signal = ctx->next_signal;

            ctx = ctx->next_signal;
        }
        else
        {
            // If this isn't the first element, make the previous element point
            // to the next one and skip the current one.
            ctx_prev->next_signal = ctx->next_signal;

            ctx_prev = ctx;
            ctx = ctx->next_signal;
        }

        count++;
    }

    if (count > 0)
    {
        int oldIME = enterCriticalSection();
        cothread_threads_waiting_count -= count;
        leaveCriticalSection(oldIME);
    }
}

//-------------------------------------------------------------------

cothread_t cothread_get_current(void)
{
    return (cothread_t)cothread_active_thread;
}

ITCM_CODE ARM_CODE static int cothread_scheduler_start(void)
{
    cothread_info_t *ctx = &cothread_list;

    while (1)
    {
        // Next context we need to switch to. We may need to delete this context
        // after returning from it, so we need to preserve the pointer to the
        // next thread.
        cothread_info_t *next_ctx = ctx->next;

        // If the thread has finished, skip it
        if (ctx->joined == 0)
        {
            // If this thread is waiting for an event (like an interrupt) to
            // happen, skip it.
            if ((ctx->flags & COTHREAD_WAITING) == 0)
            {
                // Set this thread as the active one and resume it.
                cothread_active_thread = ctx;

                set_tls(ctx->tls);

                int ret = __ndsabi_coro_resume((void *)ctx);

                // Check if the thread has just ended
                if (ctx->joined)
                {
                    // If this is the main() thread, exit the whole program with
                    // the exit code returned by main().
                    if (ctx == &cothread_list)
                        return ctx->arg;

                    // This is a regular thread.

                    // If it is detached, delete it. If not, save the exit code
                    // so that the user can check it later.
                    if (ctx->flags & COTHREAD_DETACHED)
                        cothread_delete_internal(ctx);
                    else
                        ctx->arg = ret;
                }
            }
        }

        // Get the next thread
        ctx = next_ctx;
        if (ctx == NULL)
        {
            // The end of the list has been reached. Go back to the start
            ctx = &cothread_list;

            // Whenever we reach the end of the list, check if there are threads
            // that aren't waiting for interrupts. If all threads are waiting
            // for interrupts, halt the CPU.

            // Block interrupts by setting IME to 0. This lets both the ARM7 and
            // ARM9 exit halt state if "(IE & IF) != 0". The interrupt will be
            // handled as soon as we leave the critical section.
            int oldIME = enterCriticalSection();

            // We need to check the number of active threads and enter halt
            // state atomically or it's possible that an interrupt happens right
            // before entering halt state and then there is nothing else that
            // takes us out of halt state.
            if (cothread_threads_count == cothread_threads_waiting_count)
            {
                // If no thread is active that means that all threads are
                // waiting for an event (such as interrupt) to happen. Use BIOS
                // calls to enter low power mode.
#ifdef ARM9
                // TODO: We should be able to use CP15_WaitForInterrupt(), but
                // it hangs the CPU for some reason. swiIntrWait() sets REG_IME
                // to 1 internally so it can exit halt state.

                // Wait for all IRQs enabled by the user.
                swiIntrWait(INTRWAIT_KEEP_FLAGS, REG_IE);
#elif defined(ARM7)
                swiHalt();
#endif
            }

            leaveCriticalSection(oldIME);
        }
    }
}

#ifdef ARM9
typedef struct
{
    int argc;
    char **argv;
} main_args_t;

// Allocate this in main RAM rather than the stack to save DTCM
static main_args_t main_args;
#endif

int cothread_main(void *arg)
{
    (void)arg;

    extern void __libc_init_array(void); // This is in picolibc
    extern void initSystem(void); // This is in libnds
    extern int main(int argc, char **argv, char *envp[]); // This is in user code

#ifdef ARM9
    // Initialize hardware
    initSystem();
#endif

    // Initialize global constructors after threads are working
    __libc_init_array();

#ifdef ARM9
    return main(main_args.argc, main_args.argv, NULL);
#else
    return main(0, NULL, NULL);
#endif
}

int cothread_start(int argc, char **argv, void *main_stack_top)
{
#ifdef ARM9
    main_args.argc = argc;
    main_args.argv = argv;
#endif
#ifdef ARM7
    (void)argc;
    (void)argv;
#endif

    // Initialize TLS of the main thread
    init_tls(__tls_start);

    // Thread local storage for the main thread, defined by the linker,
    // is initialized to __tls_start by the crt0.

    // The first element of cothread_list is statically allocated, used for the
    // main() thread.
    cothread_t id = cothread_create_internal(&cothread_list,
                                             cothread_main, NULL,
                                             main_stack_top, __tls_start, 0);

    // Start scheduler after everything is ready.
    cothread_scheduler_start();

    // If the scheduler returns it's because main() has returned.
    return cothread_get_exit_code(id);
}
