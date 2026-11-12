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
#include <nds/arm9/tcm_alloc.h>
#endif
#include <nds/bios.h>
#include <nds/cothread.h>
#include <nds/exceptions.h>
#include <nds/interrupts.h>
#include <nds/memory.h>
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

// Saved stack pointer of the scheduler
volatile uintptr_t DTCM_BSS_VAR(cothread_scheduler_sp);

// This is the context of the main thread, which can't be freed (if the main
// thread ends, the program ends).
static cothread_info_t DTCM_BSS_VAR(main_thread_ctx) = { 0 };

// This is the sleep thread context, which always has the lowest possible
// priority, and it runs whenever there are no more threads awake.
static cothread_info_t DTCM_BSS_VAR(sleep_thread_ctx) = { 0 };

// This is the stack used by the sleep thread.
ALIGN(8) static uint8_t DTCM_BSS_VAR(sleep_stack)[768];

// Linked lists of threads. The first entry in cothread_list_active is the
// active thread.
volatile cothread_info_t *DTCM_BSS_VAR(cothread_list_active);
volatile cothread_info_t *DTCM_BSS_VAR(cothread_list_waiting);
volatile cothread_info_t *DTCM_BSS_VAR(cothread_list_finished);

bool cothread_preemption_enabled = false;

//-------------------------------------------------------------------

// Linker symbols
extern char __tls_size[];

// This is a reference TLS data section that is used to initialize the TLS of
// threads after boot.
extern char __tdata_start[];
extern char __tdata_size[];

//extern char __tbss_start[];
extern char __tbss_size[];

void init_tls(void *__tls)
{
    // Copy reference TLS data to the new TLS context, and clear the new TLS bss

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
//
// It starts as NULL so that any usage of TLS before initializing the threading
// system causes a crash.
void *ITCM_DATA_VAR(__tls) = (char *)NULL - TCB_SIZE;

static inline void set_tls(void *tls)
{
    __tls = (uint8_t *)tls - TCB_SIZE;
}

//-------------------------------------------------------------------

static void free_anywhere(void *ptr)
{
#ifdef ARM9
    uintptr_t address = (uintptr_t)ptr;

    if ((DTCM_START <= address) && (address < DTCM_END))
    {
        dtcm_free(ptr);
        return;
    }

    if ((ITCM_START <= address) && (address < ITCM_END))
    {
        itcm_free(ptr);
        return;
    }
#endif

    free(ptr);
}

//-------------------------------------------------------------------

static void ITCM_FUNC(cothread_list_active_add_ctx)(volatile cothread_info_t *ctx)
{
    assert((REG_IME == 0) || (getCPSR() & CPSR_FLAG_IRQ_DIS));

    // Add new active contexts to the end of the list

    if (cothread_list_active == NULL)
    {
        cothread_list_active = ctx;
        ctx->next = NULL;
    }
    else
    {
        int8_t new_ctx_priority = ctx->priority;

        volatile cothread_info_t *p = cothread_list_active;

        while (1)
        {
            cothread_info_t *next = p->next;

            if (next == NULL)
                break;

            if (next->priority < new_ctx_priority)
                break;

            p = next;
        }

        ctx->next = p->next;
        p->next = (cothread_info_t *)ctx;
    }
}

static void ITCM_FUNC(cothread_list_active_remove_first_ctx)(void)
{
    assert((REG_IME == 0) || (getCPSR() & CPSR_FLAG_IRQ_DIS));
    assert(cothread_list_active);

#ifndef NDEBUG
    volatile cothread_info_t *old_head = cothread_list_active;
#endif

    // Make the head point to the next element
    cothread_list_active = cothread_list_active->next;

#ifndef NDEBUG
    old_head->next = NULL;
#endif
}

static void ITCM_FUNC(cothread_list_waiting_add_ctx)(volatile cothread_info_t *ctx)
{
    assert((REG_IME == 0) || (getCPSR() & CPSR_FLAG_IRQ_DIS));

    // Add new waiting contexts to the front of the list. It doesn't matter
    // where they are added because they all have to be checked when an
    // interrupt or signal happens.

    if (cothread_list_waiting)
        cothread_list_waiting->prev = (cothread_info_t *)ctx;

    ctx->next = (cothread_info_t *)cothread_list_waiting;
    ctx->prev = NULL;

    cothread_list_waiting = ctx;
}

static void ITCM_FUNC(cothread_list_finished_add_ctx)(volatile cothread_info_t *ctx)
{
    assert((REG_IME == 0) || (getCPSR() & CPSR_FLAG_IRQ_DIS));

    // Add new waiting contexts to the front of the list. It doesn't matter
    // where they are added because they will all be deleted at the same time.

    ctx->next = (cothread_info_t *)cothread_list_finished;

    cothread_list_finished = ctx;
}

static void ITCM_FUNC(cothread_list_waiting_remove_ctx)(volatile cothread_info_t *ctx)
{
    assert((REG_IME == 0) || (getCPSR() & CPSR_FLAG_IRQ_DIS));

    cothread_info_t *prev = ctx->prev;
    cothread_info_t *next = ctx->next;

    if (prev)
        prev->next = next;
    else
        cothread_list_waiting = next;

    if (next)
        next->prev = prev;

#ifndef NDEBUG
    ctx->next = NULL;
    ctx->prev = NULL;
#endif
}

//-------------------------------------------------------------------

#ifndef NDEBUG
static void cothread_write_canaries(volatile cothread_info_t *ctx)
{
    uint32_t *canary;

    canary = ctx->stack_top;
    canary[-1] = 0xFEDCBA98;
    canary[-2] = 0xFEDCBA98;

    canary = ctx->stack_base;
    canary[0] = 0x12345678;
    canary[1] = 0x12345678;
}

static void ITCM_FUNC(cothread_check_canaries)(volatile cothread_info_t *ctx)
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

__attribute__((noinline))
static void cothread_delete_internal(volatile cothread_info_t *ctx)
{
    if (ctx->flags & COTHREAD_ALLOCATED_STACK)
        free_fn(ctx->stack_base);

    free_fn(ctx->tls);

    if (ctx->flags & COTHREAD_ALLOCATED_CONTEXT)
        free_fn((void *)ctx);
}

static void cothread_delete_all_finished_threads(void)
{
    // Get a pointer to the start of the list and immediately set it to NULL so
    // that the scheduler can start adding more threads to it whenever it wants.
    // After that, it's safe to delete the thread contexts at our own pace.

    cpsrDisableInterrupts();

    volatile cothread_info_t *ctx = cothread_list_finished;
    cothread_list_finished = NULL;

    cpsrEnableInterrupts();

    while (ctx)
    {
        cothread_info_t *next = ctx->next;

        cothread_delete_internal(ctx);

        ctx = next;
    }
}

// Only for non-detached threads, and only for finished threads
int cothread_delete(cothread_t thread)
{
    cothread_info_t *ctx = (cothread_info_t *)thread;

    int oldIME = enterCriticalSection();

    int ret;

    if (ctx->state != COTHREAD_STATE_ENDED)
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

__attribute__((noinline))
static cothread_t cothread_create_internal(cothread_info_t *ctx,
                                           int (*entrypoint)(void *), void *arg,
                                           void *stack_base, void *stack_top,
                                           void *tls, uint16_t flags)
{
    ctx->stack_base = stack_base;
    ctx->stack_top = stack_top;
    ctx->flags = flags;
    ctx->tls = tls;
    ctx->wait_signal_id = COTHREAD_NO_SIGNAL;
    ctx->state = COTHREAD_STATE_ACTIVE;

#ifndef NDEBUG
    cothread_write_canaries(ctx);
    // Reduce size of the stack by 8 bytes to preserve the alignment to 8 bytes
    stack_top = (void *)((uint32_t *)stack_top - 2);
#endif

    // Initialize context
    cothread_context_initialize(ctx, stack_top, entrypoint, arg);

    // Add main thread to the list of active threads
    int oldIME = enterCriticalSection();
    cothread_list_active_add_ctx(ctx);
    leaveCriticalSection(oldIME);

    return (cothread_t)ctx;
}

cothread_t cothread_create_manual(int (*entrypoint)(void *), void *arg,
                                  void *stack_base, size_t stack_size,
                                  uint16_t flags)
{
    if ((stack_base == NULL) || (stack_size == 0) || (entrypoint == NULL))
        goto invalid_args;

    // AAPCS wants the stack to be aligned to 8 bytes

    if (((stack_size & 7) != 0) || (((uintptr_t)stack_base & 7) != 0))
        goto invalid_args;

    // Setup context

    cothread_info_t *ctx;

#ifdef ARM9
    if (flags & COTHREAD_CONTEXT_IN_ITCM)
        ctx = itcm_calloc(1, sizeof(cothread_info_t));
    else if (flags & COTHREAD_CONTEXT_IN_DTCM)
        ctx = dtcm_calloc(1, sizeof(cothread_info_t));
    else
#endif
        ctx = calloc(1, sizeof(cothread_info_t));

    if (ctx == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    ctx->flags |= COTHREAD_ALLOCATED_CONTEXT;

    // Create a new thread-local storage

    void *tls = malloc((size_t)__tls_size);
    if (tls == NULL)
    {
        free_anywhere(ctx);
        errno = ENOMEM;
        return -1;
    }

    init_tls(tls);

    // Child threads inherit the priority of their parent

    ctx->priority = cothread_list_active->priority;

    // Assign the free() function to the pointer because now we are sure that we
    // will need to free the resources of the newly created thread eventually.

    free_fn = free_anywhere;

    void *stack_top = (void *)((uintptr_t)stack_base + stack_size);

    return cothread_create_internal(ctx, entrypoint, arg,
                                    stack_base, stack_top, tls, flags);

invalid_args:
    errno = EINVAL;
    return -1;
}

cothread_t cothread_create(int (*entrypoint)(void *), void *arg,
                           size_t stack_size, uint16_t flags)
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
    void *stack_base;

#ifdef ARM9
    if (flags & COTHREAD_STACK_IN_ITCM)
        stack_base = itcm_memalign(8, stack_size);
    else if (flags & COTHREAD_STACK_IN_DTCM)
        stack_base = dtcm_memalign(8, stack_size);
    else
#endif
        stack_base = memalign(8, stack_size);

    if (stack_base == NULL)
    {
        errno = ENOMEM;
        return -1;
    }
    flags |= COTHREAD_ALLOCATED_STACK;

    // Create thread

    cothread_t id = cothread_create_manual(entrypoint, arg,
                                           stack_base, stack_size, flags);
    if (id == -1)
    {
        free_fn(stack_base);
        return -1;
    }

    return id;
}

int8_t cothread_get_priority(cothread_t thread)
{
    int oldIME = enterCriticalSection();

    if (thread == 0)
        thread = cothread_get_current();

    cothread_info_t *ctx = (cothread_info_t *)thread;

    int ret;

    if (ctx->state != COTHREAD_STATE_ENDED)
    {
        ret = ctx->priority;
    }
    else
    {
        errno = EINVAL;
        ret = -128;
    }

    leaveCriticalSection(oldIME);

    return ret;
}

int cothread_set_priority(cothread_t thread, int8_t priority)
{
    int oldIME = enterCriticalSection();

    if (thread == 0)
        thread = cothread_get_current();

    cothread_info_t *ctx = (cothread_info_t *)thread;

    int ret;

    if (ctx->state != COTHREAD_STATE_ENDED)
    {
        ctx->priority = priority;
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

int cothread_detach(cothread_t thread)
{
    int oldIME = enterCriticalSection();

    cothread_info_t *ctx = (cothread_info_t *)thread;

    int ret;

    if (ctx->state != COTHREAD_STATE_ENDED)
    {
        ctx->flags |= COTHREAD_DETACHED;
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

    if (ctx->state == COTHREAD_STATE_ENDED)
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

    if (ctx->state == COTHREAD_STATE_ENDED)
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

    cothread_context_switch_to_scheduler();
}

void ITCM_FUNC(cothread_yield_irq)(uint32_t mask)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
    {
        if (REG_IME == 1)
            swiIntrWait(INTRWAIT_KEEP_FLAGS, mask);

        return;
    }

    // Disable interrupts until the scheduler moves this thread to the waiting list
    cpsrDisableInterrupts();

    volatile cothread_info_t *ctx = cothread_list_active;

    ctx->wait_signal_id = COTHREAD_NO_SIGNAL;
    ctx->wait_irq_mask = mask;
    ctx->state = COTHREAD_STATE_WAITING;

    cothread_context_switch_to_scheduler();
}

#ifdef ARM7
void cothread_yield_irq_aux(uint32_t mask)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
    {
        if (REG_IME == 1)
            swiIntrWaitAUX(INTRWAIT_KEEP_FLAGS, 0, mask);

        return;
    }

    // Disable interrupts until the scheduler moves this thread to the waiting list
    cpsrDisableInterrupts();

    volatile cothread_info_t *ctx = cothread_list_active;

    ctx->wait_signal_id = COTHREAD_NO_SIGNAL;
    ctx->wait_irq_aux_mask = mask;
    ctx->state = COTHREAD_STATE_WAITING;

    cothread_context_switch_to_scheduler();
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

    // Disable interrupts until the scheduler moves this thread to the waiting list
    cpsrDisableInterrupts();

    volatile cothread_info_t *ctx = cothread_list_active;

    ctx->wait_signal_id = signal_id;
    ctx->wait_irq_mask = 0;
    ctx->state = COTHREAD_STATE_WAITING;

    cothread_context_switch_to_scheduler();
}

void ITCM_FUNC(cothread_yield_irq_or_signal)(uint32_t mask, uint32_t signal_id)
{
    // We can't yield from inside an interrupt handler
    if (irq_nesting_level > 0)
        return;

    // An ID of zero is treated as "not used"
    if (signal_id == 0)
        return;

    // Disable interrupts until the scheduler moves this thread to the waiting list
    cpsrDisableInterrupts();

    volatile cothread_info_t *ctx = cothread_list_active;

    ctx->wait_signal_id = signal_id;
    ctx->wait_irq_mask = mask;
    ctx->state = COTHREAD_STATE_WAITING;

    cothread_context_switch_to_scheduler();
}

//-------------------------------------------------------------------

// Internal function
ARM_CODE void ITCM_FUNC(cothread_send_irq)(uint32_t irq_mask)
{
    assert((REG_IME == 0) || (getCPSR() & CPSR_FLAG_IRQ_DIS));

    volatile cothread_info_t *ctx = cothread_list_waiting;

    while (ctx != NULL)
    {
        volatile cothread_info_t *next = ctx->next;

        // Check if the IRQ mask contains the received IRQ mask
        if (ctx->wait_irq_mask & irq_mask)
        {
            // TODO: Optimize removing so that we don't need a doubly linked list
            cothread_list_waiting_remove_ctx(ctx);
            cothread_list_active_add_ctx(ctx);
            ctx->state = COTHREAD_STATE_ACTIVE;
        }

        ctx = next;
    }
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
            // TODO: Optimize removing so that we don't need a doubly linked list
            cothread_list_waiting_remove_ctx(ctx);
            cothread_list_active_add_ctx(ctx);
            ctx->state = COTHREAD_STATE_ACTIVE;
        }

        ctx = next;
    }

    leaveCriticalSection(oldIME);
}

//-------------------------------------------------------------------

// This thread runs whenever there are no more active threads, its purpose is to
// always remain in active state and to sleep whenever it runs.
__attribute__((noreturn))
ARM_CODE static int ITCM_FUNC(cothread_sleep)(void *arg)
{
    (void)arg;

    while (1)
    {
        // While in the sleep thread, do garbage collection of all detached
        // threads that have ended.
        cothread_delete_all_finished_threads();

#if defined(ARM9)
        CP15_WaitForInterrupt();
#elif defined(ARM7)
        swiHalt();
#endif
        if (!cothread_preemption_enabled)
            cothread_yield();
    }
}

//-------------------------------------------------------------------

cothread_t cothread_get_current(void)
{
    return (cothread_t)cothread_list_active;
}

// The scheduler can't handle interrupts, they can only be handled from inside a
// thread. Also, the scheduler doesn't have a valid TLS, so it can't use things
// like malloc().
__attribute__ ((noinline))
ARM_CODE static int ITCM_FUNC(cothread_scheduler_start)(void)
{
    // The main and sleep threads should already be in the active list when
    // this function starts.
    assert(cothread_list_active != NULL);

    while (1)
    {
        // Get the first entry in the list of active threads and switch to it.
        // There's always an active thread (even if it's the sleep thread).
        volatile cothread_info_t *ctx = cothread_list_active;

        // Switch to its TLS space
        set_tls(ctx->tls);

        // Jump to the thread
        cothread_context_switch_to_active();

#ifndef NDEBUG
        // After returning from the thread, check stack canaries
        cothread_check_canaries(ctx);
#endif

        // Whenever we finish executing a thread, add it to the right list. Even
        // if the thread is still active, it needs to be removed from the list
        // and added back to it in the right place according to its priority.

        cothread_list_active_remove_first_ctx();

        // Check if the thread has just ended
        if (ctx->state == COTHREAD_STATE_ENDED)
        {
            // If this is the main() thread, exit the whole program with the
            // exit code returned by main().
            if (ctx == &main_thread_ctx)
                return ctx->exit_code;

            // It isn't safe to use free() from the scheduler, so we need to
            // make the sleep thread do the garbage collection of detached
            // threads that have ended.
            if (ctx->flags & COTHREAD_DETACHED)
                cothread_list_finished_add_ctx(ctx);
        }
        else if (ctx->state == COTHREAD_STATE_WAITING)
        {
            cothread_list_waiting_add_ctx(ctx);
        }
        else if (ctx->state == COTHREAD_STATE_ACTIVE)
        {
            cothread_list_active_add_ctx(ctx);
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

    // TODO: Enable this by default when libnds actually uses it
    //cothread_preemption_setup(true);

#ifdef ARM9
    return main(main_args.argc, main_args.argv, NULL);
#else
    return main(0, NULL, NULL);
#endif
}

void cothread_preemption_enable(bool enabled)
{
    cothread_preemption_enabled = enabled;
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

    // The scheduler can't handle interrupts
    cpsrDisableInterrupts();

    // Setup main thread
    // -----------------

    // Initialize TLS of the main thread
    extern char __tls_start[];
    init_tls(__tls_start);

    // Thread local storage for the main thread, defined by the linker,
    // is initialized to __tls_start by the crt0.

    // The first element of cothread_list is statically allocated, used for the
    // main() thread. It starts with priority 0 (base priority).
    cothread_create_internal(&main_thread_ctx,
                             cothread_main, NULL,
                             getHeapLimit(), main_stack_top,
                             __tls_start, 0);

    // Setup sleep thread
    // ------------------

    // Initialize TLS of the sleep thread
    extern char __tls_sleep_start[];
    init_tls(__tls_sleep_start);

    cothread_create_internal(&sleep_thread_ctx,
                             cothread_sleep, NULL,
                             &sleep_stack[0], &sleep_stack[sizeof(sleep_stack)],
                             __tls_sleep_start, 0);

    // It has the minimum possible priority so that it only runs if there are no
    // other active threads.
    sleep_thread_ctx.priority = -128;

    // Start scheduler
    // ---------------

    cothread_scheduler_start();

    // If the scheduler returns it's because main() has returned.
    return main_thread_ctx.exit_code;
}
