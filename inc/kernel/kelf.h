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
//!   from memory. If some relocations haven't been
//!   resolved, kelf_needs_fix() will return 1, and one
//!   must call kelf_fix_relocations() once all required
//!   symbols has been exported.
//! \param raw A pointer to the raw elf blob buffer
//! \return A kernel elf object, 0 if error(s) occured
kelf* kelf_load(void* raw);

//! Get the relocation state of the kernel elf.
//! \param elf The elf to work on
//! \return 0 if all relocations are satisfied in the elf,
//!         1 if not, -1 if error(s) occured
int kelf_needs_fix(kelf* elf);

//! Fix all relocations in a kernel elf
//! \param elf The elf object to patch
//! \return 0 if OK, -1 if there remains unsatisfied relocations
int kelf_fix_relocations(kelf* elf);

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
