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

#ifndef ALOS_KSYSMAP_H
#define ALOS_KSYSMAP_H

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Jump to the appropriate syscall, given its id.
//! \param id The identifier of the system call (in the above array)
//! \param a0 The first argument for the syscall (if applicable)
//! \param a1 The second argument for the syscall (if applicable)
//! \param a2 The third argument for the syscall (if applicable)
//! \return The return value of the syscall, -1 if invalid id
//!         or null syscall handler address
int ksysmap_jump(int id, void* a0, void* a1, void* a2);

#endif // ALOS_KSYSMAP_H
