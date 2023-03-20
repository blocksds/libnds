// SPDX-License-Identifier: Zlib
//
// SPDX-FileContributor: Antonio Niño Díaz, 2023

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/errno.h>

#include <ndsabi.h>

#include <nds/cothread.h>

// The stack of main() goes into DTCM, which is 16 KiB in total.
#define DEFAULT_STACK_SIZE_MAIN  (4 * 1024)
#define DEFAULT_STACK_SIZE_CHILD (1 * 1024)

// This extends __ndsabi_coro_t
typedef struct {
    uint32_t arm_sp : 31;
    uint32_t joined : 1;
    uint32_t arg;

    // Specific to libnds
    void *stack_base; // If not NULL, it has to be freed by the scheduler
    void *prev, *next;
    uint32_t flags;
} cothread_info_t;

//-------------------------------------------------------------------

static cothread_info_t *cothread_list = NULL;
static cothread_info_t *cothread_active_thread = NULL;

static void cothread_list_add_ctx(cothread_info_t *ctx)
{
    if (cothread_list == NULL) // This should only happen for main()
    {
        cothread_list = ctx;
        return;
    }

    // Find last node of the list

    cothread_info_t *p = cothread_list;

    while (p->next != NULL)
        p = p->next;

    p->next = ctx;
    ctx->prev = p;
}

static void cothread_list_remove_ctx(cothread_info_t *ctx)
{
    // cothread_list should never be NULL because main() should always be in the
    // list of active threads, so this function can be simplified.
    //
    // Also, it isn't possible to remove main(), so the first element can never
    // be removed.

    cothread_info_t *p = cothread_list->next;

    if (p == NULL)
        return;

    while (1)
    {
        if (p == ctx)
        {
            cothread_info_t *prev = p->prev;
            cothread_info_t *next = p->next;

            // There is always a previous element because main() is always there
            prev->next = next;

            // Check if this is the end of the list
            if (next != NULL)
                next->prev = prev;

            return;
        }

        if (p->next == NULL)
            break;

        p = p->next;
    }
}

static bool cothread_list_contains_ctx(cothread_info_t *ctx)
{
    cothread_info_t *p = cothread_list;

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
        free(ctx->stack_base);

    free(ctx);
}

int cothread_delete(int thread)
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

int cothread_create(int (*entrypoint)(void*, void*), void *arg,
                    void *stack_base, size_t stack_size, unsigned int flags)
{
    if (((stack_size & 3) != 0) || (((uintptr_t)stack_base & 3) != 0))
    {
        errno = EINVAL;
        return -1;
    }

    // Setup context

    cothread_info_t *ctx = calloc(sizeof(cothread_info_t), 1);
    if (ctx == NULL)
    {
        errno = ENOMEM;
        return -1;
    }

    ctx->flags = flags;

    // Setup stack
    if (stack_base == NULL)
    {
        // The user hasn't specified a memory region to use as stack. Allocate
        // it automatically.

        if (stack_size == 0)
            stack_size = DEFAULT_STACK_SIZE_CHILD;

        stack_base = malloc(stack_size);
        if (stack_base == NULL)
        {
            free(ctx);
            errno = ENOMEM;
            return -1;
        }

        ctx->stack_base = stack_base;
    }
    else
    {
        // If the user has provided the stack pointer, trust the stack pointer
        // and the size.
    }

    void *stack_top = (void *)((uintptr_t)stack_base + stack_size);

    // Add context to the scheduler
    cothread_list_add_ctx(ctx);

    // Initialize context
    __ndsabi_coro_make((void *)ctx, stack_top, (void *)entrypoint, arg);

    return (int)ctx;
}

int cothread_detach(int thread)
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

bool cothread_has_joined(int thread)
{
    cothread_info_t *ctx = (cothread_info_t *)thread;

    if (!cothread_list_contains_ctx(ctx))
    {
        errno = EINVAL;
        return false;
    }

    return ctx->joined != 0;
}

int cothread_get_exit_code(int thread)
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

void cothread_sleep(void)
{
    cothread_info_t *ctx = cothread_active_thread;

    __ndsabi_coro_yield((void *)ctx, 0);
}

//-------------------------------------------------------------------

static int cothread_scheduler_start(void)
{
    cothread_info_t *ctx = cothread_list;

    while (1)
    {
        bool delete_thread = false;

        // If the thread has finished, skip it
        if (ctx->joined == 0)
        {
            // Set this thread as the active one and resume it.
            cothread_active_thread = ctx;
            ctx->arg = __ndsabi_coro_resume((void *)ctx);

            // Check if the thread has just ended
            if (ctx->joined)
            {
                if (ctx == cothread_list)
                {
                    // If this is the main() thread, exit the whole program with
                    // the exit code returned by main().
                    return ctx->arg;
                }
                else
                {
                    // This is a regular thread, detect if this a detached
                    // thread. In that case, delete it. If not, let the user
                    // delete it after checking the exit code.
                    if (ctx->flags & COTHREAD_DETACHED)
                        delete_thread = true;
                }
            }
        }

        if (delete_thread)
        {
            cothread_info_t *ctx_to_delete = ctx;

            // Get the next thread
            ctx = ctx->next;
            if (ctx == NULL)
                ctx = cothread_list;

            cothread_delete_internal(ctx_to_delete);
        }
        else
        {
            // Get next thread normally
            ctx = ctx->next;
            if (ctx == NULL)
                ctx = cothread_list;
        }
    }
}

typedef struct {
    int argc;
    char **argv;
} main_args_t;

static main_args_t main_args;

int cothread_main(void *ctx, void *arg)
{
    main_args_t *main_args = arg;

    extern int main(int argc, char **argv);

    return main(main_args->argc, main_args->argv);
}

int cothread_start(int argc, char **argv)
{
    main_args.argc = argc;
    main_args.argv = argv;

    // For main(), allocate the stack in the regular stack (DTCM)
    uint8_t *stack = alloca(DEFAULT_STACK_SIZE_MAIN);
    uint8_t *stack_top = stack + DEFAULT_STACK_SIZE_MAIN;

    int id = cothread_create(cothread_main, &main_args, stack_top, 0, 0);

    cothread_scheduler_start();

    // If the scheduler returns it's because main() has returned.

    return cothread_get_exit_code(id);
}
