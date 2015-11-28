/*
 * alOS
 * Copyright (C) 2015 Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ALOS_KMALLOC_H
#define ALOS_KMALLOC_H

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Initialize the kernel allocator.
//! \return 0 if suceeded, -1 upon failure
int kmalloc_init();

//! Request the allocation of a block of size bytes.
//! \param size The size (in bytes) of the block to allocate
//! \return The base address of the block or 0 upon failure
//!         It is ensured to be aligned to a KMALLOC_ALIGNMENT
//!         bytes boundary (defined in the Makefile).
void* kmalloc(int size);

//! Request the reallocation of a block with a new size (copying
//!   data across the two buffers).
//! \param ptr The base address of the old block
//! \param size The new size of the block (may be < or >)
//! \return The base address of the newly allocated block or 0
//!         upon failure
void* krealloc(void* ptr, int size);

//! Release a block of memory to the allocator.
//! \param ptr The base address of the block to release
void kfree(void* ptr);

#endif // ALOS_KMALLOC_H
