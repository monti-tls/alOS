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

#include "kernel/ksymbols.h"
#include "kernel/kmalloc.h"

#include <string.h>

///////////////////////////
//// Module parameters ////
///////////////////////////

//! The kernel symbol table is always a multiple
//!   of the bulk size (in bytes) to avoid too much reallocations.
#define BULK_SIZE 32

////////////////////////////////
//// Module's sanity checks ////
////////////////////////////////

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

//! The internal kernel symbol structure.
typedef struct
{
    //!< Name of the kernel symbol (must be unique)
    const char* name;
    //!< Location of the kernel symbol
    void* location;
} symbol;

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

static int symbols_table_size = 0;
static symbol* symbols_table = 0;

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

//! Get the next free symbol slot in the table,
//!   eventually growing it
//! \return A pointer to the free symbol slot,
//!         0 if error(s) occured
static symbol* add_symbol()
{
    // Attempt to find an unused slot
    for(int i = 0; i < symbols_table_size; ++i)
    {
        if(!symbols_table[i].name || !symbols_table[i].location)
        {
            return symbols_table + i;
        }
    }

    // If we got here, we didn't found any free symbol, and so
    //   we must resize the table
    int next_id = symbols_table_size;

    // Realloc
    int new_size = symbols_table_size + BULK_SIZE;
    symbols_table = krealloc(symbols_table, new_size * sizeof(symbol));
    if(!symbols_table)
        return 0;

    // Nullify unused symbols
    for(int i = symbols_table_size; i < new_size; ++i)
    {
        symbols_table[i].name = 0;
        symbols_table[i].location = 0;
    }

    // Update size
    symbols_table_size = new_size;

    return symbols_table + next_id;
}

//! Print out the kernel's symbol table.
static void __attribute__((unused)) dump(void (*debug)(const char*, ...))
{
    int i;
    for(i = 0; i < symbols_table_size; ++i)
    {
        symbol* sym = symbols_table + i;
        if(!sym->name || !sym->location)
            continue;

        (*debug)("%25s @ %8x\n", sym->name, sym->location);
    }

    int more = symbols_table_size - i - 1;
    if(more > 0)
        (*debug)("~~~ %d more unused slots ~~~\n", more);
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////

int ksymbol_add(const char* name, void* location)
{
    if(!name || !location)
        return -1;

    // Access the next element in the symbol table
    symbol* sym = add_symbol(symbols_table_size);
    if(!sym)
        return -1;

    sym->name = name;
    sym->location = location;

    return 0;
}

int ksymbol_remove(const char* name)
{
    if(!name)
        return -1;

    for(int i = 0; i < symbols_table_size; ++i)
    {
        symbol* sym = symbols_table + i;
        if(!sym->name || !sym->location)
            continue;

        if(strcmp(sym->name, name) == 0)
        {
            sym->name = 0;
            sym->location = 0;
        }
    }

    return -1;
}

void* ksymbol(const char* name)
{
    if(!name)
        return 0;

    for(int i = 0; i < symbols_table_size; ++i)
    {
        symbol* sym = symbols_table + i;
        if(!sym->name || !sym->location)
            continue;

        if(strcmp(sym->name, name) == 0)
            return sym->location;
    }

    return 0;
}
