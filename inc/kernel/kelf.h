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

#ifndef ALOS_KELF_H
#define ALOS_KELF_H

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

//! Opaque struct representing a loaded
//!   ELF binary in the memory
typedef struct kelf kelf;

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Load and prepare for execution an elf blob
//!   from memory
//! \param raw A pointer to the raw elf blob buffer
//! \return A kernel elf object, 0 if error(s) occured
kelf* kelf_load(void* raw);

//! Unload and release a kernel elf object
//! \param elf The elf object to release
void kelf_unload(kelf* elf);

//! Get a symbol's address by name from a kernel
//!   elf object
//! \param elf The elf object from which to extract the symbol
//! \param name The name of the symbol to be resolved
//! \return The address of the resolved symbol, 0 if not found or if
//!         error(s) occured
void* kelf_symbol(kelf* elf, const char* name);

#endif // ALOS_KELF_H
