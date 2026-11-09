// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023-2026 Antonio Niño Díaz

#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARM9
#include <nds/arm9/cp15.h>
#endif
#include <nds/bios.h>
#include <nds/cothread.h>
#include <nds/exceptions.h>
#include <nds/interrupts.h>
#include <nds/ndstypes.h>
#include <nds/system.h>

#include "common/cothread/context.h"

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
volatile cothread_info_t *DTCM_BSS_VAR(cothread_active_thread) = NULL;

// Saved stack pointer of the scheduler
volatile uintptr_t DTCM_BSS_VAR(cothread_scheduler_sp);

// This is the context of the main thread, which can't be freed (if the main
// thread ends, the program ends).
static cothread_info_t main_thread_ctx;

// Linked lists of threads
volatile cothread_info_t *DTCM_BSS_VAR(cothread_list_active);
volatile cothread_info_t *DTCM_BSS_VAR(cothread_list_waiting);

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
void *ITCM_DATA_VAR(__tls) = __tls_start - TCB_SIZE;

static inline void set_tls(void *tls)
{
    __tls = (uint8_t *)tls - TCB_SIZE;
}

//-------------------------------------------------------------------

static void cothread_list_add_ctx(volatile cothread_info_t **list,
                                  volatile cothread_info_t *ctx)
{
    assert(REG_IME == 0);

    while (1)
    {
        volatile cothread_info_t *node = *list;

        if (node == NULL)
        {
            *list = ctx;
            ctx->next = NULL;
            break;
        }

        list = (volatile cothread_info_t **)&(node->next);
    }
}

static void ITCM_FUNC(cothread_list_remove_ctx)(volatile cothread_info_t **list,
                                                volatile cothread_info_t *ctx)
{
    assert(REG_IME == 0);

    while (1)
    {
        volatile cothread_info_t *node = *list;

        if (node == ctx)
        {
            // Remove from list
            *list = node->next;
            ctx->next = NULL;
            return;
        }

        if (node == NULL)
            break;

        list = (volatile cothread_info_t **)&(node->next);
    }

    // The function is trying to remove a context that isn't found in the list,
    // which means that there is a bug somewhere in the code.
    libndsCrash(__func__);
}

static bool cothread_list_contains_ctx(volatile cothread_info_t **list,
                                       volatile cothread_info_t *ctx)
{
    assert(REG_IME == 0);

    while (1)
    {
        volatile cothread_info_t *node = *list;

        if (node == ctx)
            return true;

        if (node == NULL)
            return false;

        list = (volatile cothread_info_t **)&(node->next);
    }
}

//-------------------------------------------------------------------

#ifndef NDEBUG
void cothread_write_canaries(volatile cothread_info_t *ctx)
{
    uint32_t *canary;

    canary = ctx->stack_top;
    canary[-1] = 0xFEDCBA98;
    canary[-2] = 0xFEDCBA98;

    canary = ctx->stack_base;
    canary[0] = 0x12345678;
    canary[1] = 0x12345678;
}

void cothread_check_canaries(volatile cothread_info_t *ctx)
{
    uint32_t *canary;

    canary = ctx->stack_top;
    if ((canary[-1] != 0xFEDCBA98) || (canary[-2] != 0xFEDCBA98))
        libndsCrash("Thread stack underflow");

    canary = ctx->stack_base;
    if ((canary[0] != 0x12345678) || (canary[1] != 0x12345678))
        libndsCrash("Thread stack overflow");
}
#endif // NDEBUG

//-------------------------------------------------------------------

// This is private, only available inside libnds
void cothread_main_thread_set_stack_base(void *base)
{
    main_thread_ctx.stack_base = base;

#ifndef NDEBUG
    cothread_write_canaries(&main_thread_ctx);
#endif
}

//-------------------------------------------------------------------

static void cothread_delete_internal(volatile cothread_info_t *ctx)
{
    assert(REG_IME == 0);

    if (ctx->flags_resources & COTHREAD_ALLOCATED_STACK)
        free_fn(ctx->stack_base);

    free(ctx->tls);

    if (ctx->flags_resources & COTHREAD_ALLOCATED_CONTEXT)
        free_fn((void *)ctx);
}

// Only for non-detached threads, and only for finished threads
int cothread_delete(cothread_t thread)
{
    cothread_info_t *ctx = (cothread_info_t *)thread;

    int oldIME = enterCriticalSection();

    int ret;

    if (ctx->arm_sp != 0)
    {
        errno = EINVAL;
        ret = -1;
    }
    else
    {
        cothread_delete_internal(ctx);
        ret = 0;
    }

    leaveCriticalSection(oldIME);

    return ret;
}

static cothread_t cothread_create_internal(cothread_info_t *ctx,
                                           int (*entrypoint)(void *), void *arg,
                                           void *stack_top, void *tls,
                                           uint8_t flags)
{
    ctx->stack_top = stack_top;
    ctx->flags_user = flags;
    ctx->tls = tls;
    ctx->wait_signal_id = COTHREAD_NO_SIGNAL;

#ifndef NDEBUG
    cothread_write_canaries(ctx);
    // Reduce size of the stack by 8 bytes to preserve the alignment to 8 bytes
    stack_top = (void *)((uint32_t *)stack_top - 2);
#endif

    // Initialize context
    cothread_context_initialize(ctx, stack_top, entrypoint, arg);

    return (cothread_t)ctx;
}

cothread_t cothread_create_manual(int (*entrypoint)(void *), void *arg,
                                  void *stack_base, size_t stack_size,
                                  uint8_t flags)
{
    // stack_size can be zero, like for the main() thread.

    if ((stack_base == NULL) || (entrypoint == NULL))
        goto invalid_args;

    // AAPCS wants the stack to be aligned to 8 bytes

    if (((stack_size & 7) != 0) || (((uintptr_t)stack_base & 7) != 0))
        goto invalid_args;

    // Setup context

    cothread_info_t *ctx = calloc(1, sizeof(cothread_info_t));
    if (ctx == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    ctx->flags_resources |= COTHREAD_ALLOCATED_CONTEXT;

    // Create a new thread-local storage

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
    int oldIME = enterCriticalSection();
    cothread_list_add_ctx(&cothread_list_active, ctx);
    leaveCriticalSection(oldIME);

    void *stack_top = (void *)((uintptr_t)stack_base + stack_size);

    ctx->stack_base = stack_base;

    return cothread_create_internal(ctx, entrypoint, arg, stack_top, tls, flags);

invalid_args:
    errno = EINVAL;
    return -1;
}

cothread_t cothread_create(int (*entrypoint)(void *), void *arg,
                           size_t stack_size, uint8_t flags)
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
    ctx->flags_resources |= COTHREAD_ALLOCATED_STACK;

    return id;
}

int cothread_detach(cothread_t thread)
{
    int oldIME = enterCriticalSection();

    cothread_info_t *ctx = (cothread_info_t *)thread;

    bool allowed = true;

    if (!cothread_list_contains_ctx(&cothread_list_active, ctx))
    {
        if (!cothread_list_contains_ctx(&cothread_list_waiting, ctx))
            allowed = false;
    }

    int ret;

    if (allowed)
    {
        ctx->flags_user |= COTHREAD_DETACHED;
        ret = 0;
    }
    else
    {
        errno = EINVAL;
        ret = -1;
    }

    leaveCriticalSection(oldIME);

    return ret;
}

bool cothread_has_joined(cothread_t thread)
{
    bool joined;
    cothread_info_t *ctx = (cothread_info_t *)thread;

    int oldIME = enterCriticalSection();

    if (ctx->arm_sp == 0)
    {
        joined = true;
    }
    else
    {
        errno = EINVAL;
        joined = false;
    }

    leaveCriticalSection(oldIME);

    return joined;
}

int cothread_get_exit_code(cothread_t thread)
{
    int ret;
    cothread_info_t *ctx = (cothread_info_t *)thread;

    int oldIME = enterCriticalSection();

    if (ctx->arm_sp == 0)
    {
        ret = ctx->exit_code;
    }
    else
    {
        errno = EINVAL;
        ret = -1;
    }

    leaveCriticalSection(oldIME);

    return ret;
}

extern uint16_t irq_nesting_level;

void cothread_yield(void)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
        return;

    cothread_context_yield();
}

ARM_CODE void cothread_yield_irq(uint32_t mask)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
    {
        if (REG_IME == 1)
            swiIntrWait(INTRWAIT_KEEP_FLAGS, mask);

        return;
    }

    volatile cothread_info_t *ctx = cothread_active_thread;

    REG_IME = 0;

    ctx->wait_signal_id = COTHREAD_NO_SIGNAL;
    ctx->wait_irq_mask = mask;

    cothread_list_remove_ctx(&cothread_list_active, ctx);
    cothread_list_add_ctx(&cothread_list_waiting, ctx);

    // We're going to wait for an IRQ. Make sure that IRQs are enabled.
    REG_IME = 1;

    cothread_context_yield();
}

#ifdef ARM7
ARM_CODE void cothread_yield_irq_aux(uint32_t mask)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
    {
        if (REG_IME == 1)
            swiIntrWaitAUX(INTRWAIT_KEEP_FLAGS, 0, mask);

        return;
    }

    volatile cothread_info_t *ctx = cothread_active_thread;

    REG_IME = 0;

    ctx->wait_signal_id = COTHREAD_NO_SIGNAL;
    ctx->wait_irq_aux_mask = mask;

    cothread_list_remove_ctx(&cothread_list_active, ctx);
    cothread_list_add_ctx(&cothread_list_waiting, ctx);

    // We're going to wait for an IRQ. Make sure that IRQs are enabled.
    REG_IME = 1;

    cothread_context_yield();
}
#endif

void ITCM_FUNC(cothread_yield_signal)(uint32_t signal_id)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
        return;

    assert(signal_id != COTHREAD_NO_SIGNAL);
    if (signal_id == COTHREAD_NO_SIGNAL)
        return;

    volatile cothread_info_t *ctx = cothread_active_thread;

    int oldIME = enterCriticalSection();

    ctx->wait_signal_id = signal_id;
    ctx->wait_irq_mask = 0;

    cothread_list_remove_ctx(&cothread_list_active, ctx);
    cothread_list_add_ctx(&cothread_list_waiting, ctx);

    leaveCriticalSection(oldIME);

    cothread_context_yield();
}

void ITCM_FUNC(cothread_yield_irq_or_signal)(uint32_t mask, uint32_t signal_id)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
        return;

    // An ID of zero is treated as "not used"
    if (signal_id == 0)
        return;

    volatile cothread_info_t *ctx = cothread_active_thread;

    int oldIME = enterCriticalSection();

    ctx->wait_signal_id = signal_id;
    ctx->wait_irq_mask = mask;

    cothread_list_remove_ctx(&cothread_list_active, ctx);
    cothread_list_add_ctx(&cothread_list_waiting, ctx);

    leaveCriticalSection(oldIME);

    cothread_context_yield();
}

//-------------------------------------------------------------------

// Internal function
ARM_CODE void ITCM_FUNC(cothread_send_irq)(uint32_t irq_mask)
{
    int oldIME = enterCriticalSection();

    volatile cothread_info_t *ctx = cothread_list_waiting;

    while (ctx != NULL)
    {
        volatile cothread_info_t *next = ctx->next;

        // Check if the IRQ mask contains the received IRQ mask
        if (ctx->wait_irq_mask & irq_mask)
        {
            cothread_list_remove_ctx(&cothread_list_waiting, ctx);
            cothread_list_add_ctx(&cothread_list_active, ctx);
        }

        ctx = next;
    }

    leaveCriticalSection(oldIME);
}

void ITCM_FUNC(cothread_send_signal)(uint32_t signal_id)
{
    int oldIME = enterCriticalSection();

    volatile cothread_info_t *ctx = cothread_list_waiting;

    while (ctx != NULL)
    {
        volatile cothread_info_t *next = ctx->next;

        // Check if the signal ID matches
        if (ctx->wait_signal_id == signal_id)
        {
            cothread_list_remove_ctx(&cothread_list_waiting, ctx);
            cothread_list_add_ctx(&cothread_list_active, ctx);
        }

        ctx = next;
    }

    leaveCriticalSection(oldIME);
}

//-------------------------------------------------------------------

cothread_t cothread_get_current(void)
{
    return (cothread_t)cothread_active_thread;
}

ARM_CODE static int ITCM_FUNC(cothread_scheduler_start)(void)
{
    // We expect at least one active thread when this function starts.

    // All threads in cothread_list_active are active (not waiting or ended)
    volatile cothread_info_t *ctx = cothread_list_active;

    assert(ctx != NULL);

    while (1)
    {
        // Set this thread as the active one and resume it.
        cothread_active_thread = ctx;
        set_tls(ctx->tls);

        cothread_context_resume();

#ifndef NDEBUG
        cothread_check_canaries(ctx);
#endif

        // Next context we need to switch to. We may need to delete this
        // context, so we need to preserve the pointer to the next thread.
        cothread_info_t *next_ctx = ctx->next;

        // Check if the thread has just ended
        if (ctx->arm_sp == 0)
        {
            // If this is the main() thread, exit the whole program with the
            // exit code returned by main().
            if (ctx == &main_thread_ctx)
                return ctx->exit_code;

            int oldIME = enterCriticalSection();

            // Remove this thread from the list of active threads regardless of
            // whether it's detached or not.
            cothread_list_remove_ctx(&cothread_list_active, ctx);

            // If it is detached, delete it.
            if (ctx->flags_user & COTHREAD_DETACHED)
                cothread_delete_internal(ctx);

            leaveCriticalSection(oldIME);
        }

        // Get the next thread
        ctx = next_ctx;

        // Check if the end of the list of active threads has been reached
        if (ctx == NULL)
        {
            // Whenever we reach the end of the list, check if there are any
            // active threads. If cothread_list_active is NULL, there are none
            // (they are all waiting for events or they have ended). If it isn't
            // NULL, there is at least one active thread.
            while (1)
            {
                int oldIME = enterCriticalSection();

                if (cothread_list_active != NULL)
                {
                    ctx = (cothread_info_t*)cothread_list_active;

                    leaveCriticalSection(oldIME);
                    break;
                }

                // No active threads. Halt the CPU using the BIOS
#if defined(ARM9)
                // TODO: We should be able to use CP15_WaitForInterrupt(), but
                // it hangs the CPU for some reason. swiIntrWait() sets REG_IME
                // to 1 internally so it can exit halt state.

                // Wait for all IRQs enabled by the user.
                swiIntrWait(INTRWAIT_KEEP_FLAGS, REG_IE);
#elif defined(ARM7)
                swiHalt();
#endif
                leaveCriticalSection(oldIME);
            }
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
    main_thread_ctx.stack_base = getHeapLimit();
    cothread_t id = cothread_create_internal(&main_thread_ctx,
                                             cothread_main, NULL,
                                             main_stack_top, __tls_start, 0);

    int oldIME = enterCriticalSection();

    // Add main thread to the list of active threads
    cothread_list_add_ctx(&cothread_list_active, &main_thread_ctx);

    leaveCriticalSection(oldIME);

    // Start scheduler after everything is ready.
    cothread_scheduler_start();

    // If the scheduler returns it's because main() has returned.
    return cothread_get_exit_code(id);
}
