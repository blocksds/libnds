// SPDX-License-Identifier: Zlib
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/errno.h>

#include <ndsabi.h>

#include <nds/bios.h>
#include <nds/cothread.h>
#include <nds/interrupts.h>
#include <nds/ndstypes.h>

// The stack of main() goes into DTCM, which is 16 KiB in total.
#define DEFAULT_STACK_SIZE_MAIN  (1 * 1024)
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

//-------------------------------------------------------------------

#ifdef ARM9
DTCM_BSS
#endif
volatile uint32_t cothread_irq_flags;

#ifdef ARM7
volatile uint32_t cothread_irq_aux_flags;
#endif

// This function returns true if there is any thread that isn't waiting for an
// interrupt to happen, false otherwise.
#ifdef ARM9
ITCM_CODE
#endif
bool cothread_scheduler_refresh_irq_flags(void)
{
    bool any_thread_available = false;

    // We need to fetch and clear the current flags in a critical section in
    // case there is an interrupt right when we are reading and clearing the
    // variable.

    int oldIME = enterCriticalSection();

    uint32_t flags = cothread_irq_flags;
    cothread_irq_flags = 0;
#ifdef ARM7
    uint32_t flags_aux = cothread_irq_aux_flags;
    cothread_irq_aux_flags = 0;
#endif

    leaveCriticalSection(oldIME);

    cothread_info_t *p = &cothread_list;

    for (; p != NULL; p = p->next)
    {
        if (p->wait_irq_flags)
            p->wait_irq_flags &= ~flags;
#ifdef ARM7
        if (p->wait_irq_aux_flags)
            p->wait_irq_aux_flags &= ~flags_aux;
#endif

        if (p->wait_irq_flags == 0)
            any_thread_available = true;
#ifdef ARM7
        if (p->wait_irq_aux_flags == 0)
            any_thread_available = true;
#endif
    }

    return any_thread_available;
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

#ifdef ARM9
ITCM_CODE
#endif
static void cothread_list_remove_ctx(cothread_info_t *ctx)
{
    // The first element of cothread_list is statically allocated. It is the
    // main() thread, which can never be deleted.

    cothread_info_t *p = &cothread_list;

    for ( ; p->next != NULL; p = p->next)
    {
        if (p->next == ctx)
        {
            // Skip the context that we have just found
            p->next = ((cothread_info_t *)p->next)->next;
            return;
        }
    }

    // Reaching this point means that there is a bug somewhere in the code.
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

    return (cothread_t)ctx;
}

cothread_t cothread_create_manual(int (*entrypoint)(void *), void *arg,
                                  void *stack_base, size_t stack_size,
                                  unsigned int flags)
{
    // stack_size can be zero, like for the main() thread.

    if ((stack_base == NULL) || (entrypoint == NULL))
        goto invalid_args;

    // They must be aligned to 32 bit
    if (((stack_size & 3) != 0) || (((uintptr_t)stack_base & 3) != 0))
        goto invalid_args;

    // Setup context

    cothread_info_t *ctx = calloc(sizeof(cothread_info_t), 1);
    if (ctx == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    // Defined by the linker
    extern char __tls_start[];
    extern char __tls_end[];
    size_t __tls_size = (uintptr_t)__tls_end - (uintptr_t)__tls_start;

    void *tls = malloc(__tls_size);
    if (tls == NULL)
    {
        free(ctx);
        errno = ENOMEM;
        return -1;
    }

    extern void _init_tls(void *__tls);
    _init_tls(tls);

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

    if ((stack_size & 3) != 0)
    {
        errno = EINVAL;
        return -1;
    }

    if (stack_size == 0)
        stack_size = DEFAULT_STACK_SIZE_CHILD;

    void *stack_base = malloc(stack_size);
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

void cothread_yield(void)
{
    cothread_info_t *ctx = cothread_active_thread;

    __ndsabi_coro_yield((void *)ctx, 0);
}

void cothread_yield_irq(uint32_t flags)
{
    cothread_info_t *ctx = cothread_active_thread;

    ctx->wait_irq_flags = flags;

    __ndsabi_coro_yield((void *)ctx, 0);
}

#ifdef ARM7
void cothread_yield_irq_aux(uint32_t flags)
{
    cothread_info_t *ctx = cothread_active_thread;

    ctx->wait_irq_aux_flags = flags;

    __ndsabi_coro_yield((void *)ctx, 0);
}
#endif

//-------------------------------------------------------------------

cothread_t cothread_get_current(void)
{
    return (cothread_t)cothread_active_thread;
}

#ifdef ARM9
ITCM_CODE
#endif
static int cothread_scheduler_start(void)
{
    cothread_info_t *ctx = &cothread_list;

    while (1)
    {
        bool delete_current_thread = false;
        cothread_info_t *ctx_to_delete = NULL;

        // If the thread has finished, skip it
        if (ctx->joined)
            goto next_thread;

        // If this thread is waiting for any interrupt to happen, skip it
#ifdef ARM9
        if (ctx->wait_irq_flags)
            goto next_thread;
#elif defined(ARM7)
        if (ctx->wait_irq_flags || ctx->wait_irq_aux_flags)
            goto next_thread;
#endif

        // Set this thread as the active one and resume it.
        cothread_active_thread = ctx;

        extern void _set_tls(void *tls);
        _set_tls(ctx->tls);

        int ret = __ndsabi_coro_resume((void *)ctx);

        // Check if the thread has just ended
        if (ctx->joined)
        {
            // If this is the main() thread, exit the whole program with the
            // exit code returned by main().
            if (ctx == &cothread_list)
                return ctx->arg;

            // This is a regular thread.

            // If it is detached, delete it. If not, save the exit code so that
            // the user can check it later.
            if (ctx->flags & COTHREAD_DETACHED)
                delete_current_thread = true;
            else
                ctx->arg = ret;
        }

next_thread:

        if (delete_current_thread)
            ctx_to_delete = ctx;

        bool any_thread_active = true;

        // Get the next thread
        ctx = ctx->next;
        if (ctx == NULL)
        {
            ctx = &cothread_list;
            any_thread_active = cothread_scheduler_refresh_irq_flags();
        }

        if (delete_current_thread)
            cothread_delete_internal(ctx_to_delete);

        if (any_thread_active == false)
        {
            // If no thread is active that means that all threads are
            // waiting for an interrupt to happen. Use BIOS calls to enter
            // low power mode.
#ifdef ARM9
            swiWaitForIRQ();
#elif defined(ARM7)
            swiHalt();
#endif
        }
    }
}

#ifdef ARM9
typedef struct {
    int argc;
    char **argv;
} main_args_t;

// Allocate this in main RAM rather than the stack to save DTCM
static main_args_t main_args;
#endif

int cothread_main(void *arg)
{
    extern int main(int argc, char **argv);

#ifdef ARM9
    return main(main_args.argc, main_args.argv);
#else
    return main(0, NULL);
#endif
}

int cothread_start(int argc, char **argv)
{
#ifdef ARM9
    main_args.argc = argc;
    main_args.argv = argv;
#endif

    // For main(), allocate the stack in the regular stack (DTCM)
    uint8_t *stack = alloca(DEFAULT_STACK_SIZE_MAIN);
    uint8_t *stack_top = stack + DEFAULT_STACK_SIZE_MAIN;

    // Thread local storage for the main thread, defined by the linker
    extern char __tls_start[];
    extern void _init_tls(void *__tls);
    _init_tls(__tls_start);

    // The first element of cothread_list is statically allocated, used for the
    // main() thread.
    cothread_t id = cothread_create_internal(&cothread_list,
                                             cothread_main, NULL,
                                             stack_top, __tls_start, 0);

    cothread_scheduler_start();

    // If the scheduler returns it's because main() has returned.

    return cothread_get_exit_code(id);
}
