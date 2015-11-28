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

#ifndef ALOS_KSYMBOLS_H
#define ALOS_KSYMBOLS_H

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Register a symbol in the kernel's symbol table.
//! \param name A null-terminated string containing
//!             the name of the symbol to register
//! \param location A pointer to the symbol
//! \return 0 on success, -1 otherwise
int ksymbol_add(const char* name, void* location);

//! Remove a symbol from the kernel's symbol table.
//! \param name The name of the symbol to remove
//! \return 0 if success, -1 otherwise
int ksymbol_remove(const char* name);

//! Search for a symbol in the table.
//! \param name The name of the symbol to resolve
//! \return A pointer to the found symbol if found,
//!         0 otherwise
void* ksymbol(const char* name);

#endif // ALOS_KSYMBOLS_H
