// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

// strverscmp() is a GNU extension
#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int alphasort(const struct dirent **a, const struct dirent **b)
{
    return strcoll((*a)->d_name, (*b)->d_name);
}

int versionsort(const struct dirent **a, const struct dirent **b)
{
    return strverscmp((*a)->d_name, (*b)->d_name);
}

int scandir(const char *path, struct dirent ***names,
            int (*filter_f)(const struct dirent *),
            int (*compare_f)(const struct dirent **, const struct dirent **))
{
    bool error = false;
    int count = 0;
    struct dirent *ent;

    DIR *dir = opendir(path);
    if (dir == NULL)
        return -1;

    *names = NULL;
    while ((ent = readdir(dir)) != NULL)
    {
        if (errno)
        {
            error = true;
            break;
        }

        if (filter_f == NULL || (filter_f(ent) != 0))
        {
            struct dirent **new_names = realloc(*names, (count + 1) * sizeof(struct dirent *));
            if (new_names == NULL)
            {
                error = true;
                break;
            }

            struct dirent *ent_copy = malloc(sizeof(struct dirent));
            if (ent_copy == NULL)
            {
                free(new_names);
                error = true;
                break;
            }
            memcpy(ent_copy, ent, sizeof(struct dirent));

            new_names[count++] = ent_copy;
            *names = new_names;
        }
    }

    if (*names != NULL && error)
    {
        // deallocate name list
        for (int i = 0; i < count; i++)
            if (*names[i] != NULL)
                free(*names[i]);
        free(*names);
        *names = NULL;
    }
    else if (count > 0 && compare_f != NULL)
    {
        // sort name list
        qsort(*names, count, sizeof(struct dirent *), (__compar_fn_t) compare_f);
    }

    closedir(dir);
    return error ? -1 : count;
}
