// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
//
// Copyright (C) 2005 Jason Rogers (dovoto)

/// @file nds/arm9/dynamicArray.h
///
/// @brief A dynamically resizing array for general use.

#ifndef LIBNDS_NDS_ARM9_DYNAMICARRAY_H__
#define LIBNDS_NDS_ARM9_DYNAMICARRAY_H__

#include <stdlib.h>
#include <string.h>

#include <nds/ndstypes.h>

/// A resizable array
typedef struct DynamicArray
{
    void **data;           ///< Pointer to array of void pointers.
    unsigned int cur_size; //!< Currently allocated size of the array.
} DynamicArray;

/// Initializes an array with the supplied initial size.
///
/// @param v The array to initialize.
/// @param initialSize The initial size to allocate.
/// @return A pointer to the data, or NULL on error.
void *DynamicArrayInit(DynamicArray *v, unsigned int initialSize);

/// Frees memory allocated by the dynamic array.
///
/// @param v The array to delete
void DynamicArrayDelete(DynamicArray* v);

/// Gets the entry at the supplied index.
///
/// @param v The array to get from.
/// @param index The index of the data to get.
/// @return The data or NULL if v is NULL or the index is out of range.
void* DynamicArrayGet(DynamicArray* v, unsigned int index);

/// Sets the entry to the supplied value.
///
/// @param v The array to set
/// @param index The index of the data to set (array will be resized to fit the
///              index).
/// @param item The data to set.
/// @return Returns false if v is NULL or there isn't enough memory, true
/// otherwise.
bool DynamicArraySet(DynamicArray *v, unsigned int index, void* item);

#endif // LIBNDS_NDS_ARM9_DYNAMICARRAY_H__
