// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2008 Jason Rogers (dovoto)

// A simple linked list data structure

/// @file nds/arm9/linkedlist.h
///
/// @brief A simple doubly linked, unsorted list implementation.

#ifndef LIBNDS_NDS_ARM9_LINKEDLIST_H__
#define LIBNDS_NDS_ARM9_LINKEDLIST_H__

/// A node of the linked list.
typedef struct LinkedList {
    struct LinkedList *next; ///< A pointer to the next node.
    struct LinkedList *prev; ///< A pointer to the previous node.
    void *data;              ///< A pointer to some data.
} LinkedList;

/// Adds data to a linked list.
///
/// This will only store the pointer to the data, so you have to make sure that
/// the pointer stays valid.
///
/// @param front A pointer to a pointer to the front of the linked list (or a
///              pointer to NULL if you don't have a linked list yet).
/// @param data A pointer to the data you want to store.
/// @return A pointer to the new node, which is also the new front, or NULL if
/// there is not enough memory.
LinkedList* linkedlistAdd(LinkedList **front, void* data);

/// Removes a node from a linked list.
///
/// The data pointer of the node will be lost after this, so make sure you don't
/// need it anymore.
///
/// @param node The node you want to remove.
void linkedlistRemove(LinkedList *node);

#endif // LIBNDS_NDS_ARM9_LINKEDLIST_H__
