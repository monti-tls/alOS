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

#include "kernel/ksysmap.h"

#define INCLUDES
#include "kernel/sysmap.h"
#undef INCLUDES

///////////////////////////
//// Module parameters ////
///////////////////////////

// N/A

////////////////////////////////
//// Module's sanity checks ////
////////////////////////////////

// N/A

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

// N/A

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

// N/A

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

//! This array contains function pointers
//!   to registered kernel system calls.
//! System call ids are the handler's address
//!   in this table
static void* ksysmap[] = {
// We want the syscalls definitions
#define SYSCALLS
// Just take the address of the system call
#define DECL_SYSCALL(rtp, symbol, args) (&(symbol)),
#include "kernel/sysmap.h"
// Cleanup
#undef DECL_SYSCALL
#undef SYSCALLS
};

//! Contains the size of the above array
static int ksysmap_size = sizeof(ksysmap);

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

// N/A

/////////////////////////////
//// Public module's API ////
/////////////////////////////

int ksysmap_jump(int id, void* a0, void* a1, void* a2)
{
    if (id >= ksysmap_size || !ksysmap[id])
        return -1;

    return ((int (*)(void*, void*, void*))ksysmap[id])(a0, a1, a2);
}
