// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008 Jason Rogers (dovoto)

#include <stdlib.h>

#include <nds/arm9/linkedlist.h>

LinkedList *linkedlistAdd(LinkedList **front, void *data)
{
    LinkedList *node = malloc(sizeof(LinkedList));

    if (node == NULL)
        return NULL;

    node->prev = NULL;
    node->data = data;

    if (*front == NULL)
    {
        node->next = NULL;
        *front = node;
    }
    else
    {
        node->next = *front;
        (*front)->prev = node;
    }

    return node;
}

void linkedlistRemove(LinkedList *node)
{
    if (node == NULL)
        return;

    if (node->prev != NULL)
        node->prev->next = node->next;

    if (node->next != NULL)
        node->next->prev = node->prev;

    free(node);
}
