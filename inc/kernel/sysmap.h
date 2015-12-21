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

#ifdef INCLUDES

// Put here necessary syscall handler kernel headers

#include "kernel/kmalloc.h"
#include "kernel/kmodule.h"
#include "kernel/ksched.h"

#endif // INCLUDES

#ifdef SYSCALLS

// Declare here each system call
// *IMPORTANT* to remain backward-compatible,
//   always keep the same syscall order,
//   and append new syscalls to the list.
// If one was removed, replace its entry by a null
//   pointer (that will return an error to the task)

DECL_SYSCALL(void*, kmalloc, (int))
DECL_SYSCALL(void*, krealloc, (void*, int))
DECL_SYSCALL(void, kfree, (void*))
DECL_SYSCALL(int, kmodule_insert, (const char*, int))
DECL_SYSCALL(int, kmodule_remove, (const char*, int))
DECL_SYSCALL(int, ksched_spawn, (const char*, void*, void*))

#endif // SYSCALLS