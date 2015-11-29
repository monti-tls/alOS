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

#include "kernel/kmodule.h"
#include "kernel/kmalloc.h"
#include "kernel/kprint.h"
#include "kernel/kelf.h"
#include "kernel/fs/vfs.h"
#include <string.h>

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

struct kmodule
{
    kelf* elf;

    const char* name;
    int* ver;
    const char* ver_string;
    const char** depends;
    int* depends_size;

    int (*init)();
    int (*fini)();

    kmodule* next;
};

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

// N/A

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

//! First element of the internal module list
static struct kmodule* module_list_first = 0;

//! Last element of the internal module list
static struct kmodule* module_list_last = 0;

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

//! Append a module to the internal loaded modules list
//! \param mod The module to add to the list
//! \return 0 if OK, -1 otherwise
static int module_list_add(kmodule* mod)
{
    if(!mod)
        return -1;

    if(!module_list_last)
    {
        module_list_first = module_list_last = mod;
    }
    else
    {
        module_list_last->next = mod;
        module_list_last = mod;
    }
    
    mod->next = 0;

    return 0;
}

//! Remove a module from the internal linked list
//! \param mod The module to remove
//! \return 0 if OK, -1 otherwise
static int module_list_remove(kmodule* mod)
{
    if(!mod)
        return -1;

    kmodule* last = 0;
    for(kmodule* m = module_list_first; m; last = m, m = m->next)
    {
        if(m == mod)
        {
            // We remove the last module
            if(m == module_list_last)
                module_list_last = last;

            // We remove the first module
            if(!last)
                module_list_first = m->next;
            // Otherwise
            else
                last->next = m->next;

            m->next = 0;
            return 0;
        }
    }

    return -1;
}

//! Find a module by name in the list
//! \param name The name to find
//! \return The module if found, 0 otherwise
static kmodule* module_list_by_name(const char* name)
{
    if(!name)
        return 0;

    for(kmodule* m = module_list_first; m; m = m->next)
    {
        if(strcmp(m->name, name) == 0)
            return m;
    }

    return 0;
}

//! Get the module's needed symbol's addresses
//! \param mod The module to work on
//! \return 0 if all symbols has been resolved, -1 otherwise
static int get_symbols(struct kmodule* mod)
{
    if(!mod || !mod->elf)
        return -1;

    mod->name = (const char*)kelf_symbol(mod->elf, "mod_name");
    mod->ver = (int*)kelf_symbol(mod->elf, "mod_ver");
    mod->ver_string = (const char*)kelf_symbol(mod->elf, "mod_ver_string");
    mod->depends = (const char**)kelf_symbol(mod->elf, "mod_depends");
    mod->depends_size = (int*)kelf_symbol(mod->elf, "mod_depends_size");

    mod->init = (int (*)())kelf_symbol(mod->elf, "mod_init");
    mod->fini = (int (*)())kelf_symbol(mod->elf, "mod_fini");

    if(mod->name && mod->ver && mod->ver_string && mod->depends && mod->depends_size && mod->init && mod->fini)
        return 0;

    return -1;
}

//! Load and initialize a module in the kernel, without any
//!   dependency checking
//! \param mod The module to insert
//! \return 0 if OK, -1 otherwise
int insert_raw(kmodule* mod)
{
    if(!mod || !mod->init)
        return -1;

    if(mod->init() < 0)
        return -1;

    return 0;
}

//! Remove a module from the kernel, without any depencency
//!   checking
//! \param mod The module to remove
//! \return 0 if OK, -1 otherwise
int remove_raw(kmodule* mod)
{
    if(!mod || !mod->fini)
        return -1;

    int err = mod->fini();
    kfree(mod);

    return err < 0 ? -1 : 0;
}

//! Insert a module into the kernel, checking its dependencies
//!   (and optionnally inserting them)
//! \param elf The elf blob to insert
//! \param load_dependencies Also load the
//!        (eventually) needed dependencies
kmodule* insert(kelf* elf, int load_dependencies)
{
    if(!elf)
        return 0;

    // Allocate memory
    kmodule* mod = kmalloc(sizeof(struct kmodule));
    if(!mod)
        return 0;
    mod->elf = elf;

    // Resolve mandatory module symbols
    if(get_symbols(mod) < 0)
    {
        kprint(KPRINT_ERR "    module '%s' not loaded: malformed symbols\n", mod->name);
        kelf_unload(mod->elf);
        kfree(mod);
        return 0;
    }

    // Dependency checking
    for(int i = 0; i < *mod->depends_size; ++i)
    {
        const char* dep = mod->depends[i];
        if(!dep)
        {
            kelf_unload(mod->elf);
            kfree(mod);
            return 0;
        }

        kmodule* depmod = module_list_by_name(dep);
        if(!depmod)
        {
            int err = 0;

            // Don't load anything as requested
            if(!load_dependencies)
            {
                err = 1;
            }
            // Attempt to load the dependency (recursively)
            else
            {
                kprint(KPRINT_TRACE "    loading dependency '%s'\n", dep);
                if(kmodule_insert(dep, 1) == 0)
                {
                    err = 1;
                }
            }

            // Dependency failure
            if(err)
            {
                kprint(KPRINT_ERR "    module '%s' not loaded: unresolved dependency '%s'\n", mod->name, dep);
                kelf_unload(mod->elf);
                kfree(mod);
                return 0;
            }
        }
    }

    if (kelf_needs_fix(mod->elf))
    {
        if (kelf_fix_relocations(mod->elf) < 0)
        {
            kprint(KPRINT_ERR "    module '%s' not loaded: unsatisfied relocations\n", mod->name);
            kelf_unload(mod->elf);
            kfree(mod);
            return 0;
        }

        get_symbols(mod);
    }

    if(insert_raw(mod) < 0)
    {
        kprint(KPRINT_ERR "    module '%s' not loaded: internal error\n", mod->name);
        kelf_unload(mod->elf);
        kfree(mod);
        return 0;
    }

    if(module_list_add(mod) < 0)
    {
        kprint(KPRINT_ERR "    module '%s' not loaded: unable to add to list\n", mod->name);
        kelf_unload(mod->elf);
        kfree(mod);
        return 0;
    }

    kprint(KPRINT_TRACE "    loaded module '%s'\n", mod->name);
    return mod;
}

//! Remove a module from the kernel
int remove(kmodule* mod, int unload_dependencies)
{
    if(!mod)
        return -1;

    // Find all reverse dependencies
    for(kmodule* m = module_list_first; m; m = m->next)
    {
        for(int i = 0; i < *m->depends_size; ++i)
        {
            const char* dep = m->depends[i];
            if(strcmp(dep, mod->name) == 0)
            {
                int err = 0;

                // Don't unload anything
                if(!unload_dependencies)
                {
                    err = 1;
                }
                // Unload the dependency
                else
                {
                    kprint(KPRINT_TRACE "    unloading reverse dependency '%s'\n", m->name);
                    if(remove(m, 1) < 0)
                    {
                        err = 1;
                    }
                }

                // Reverse dependency failure
                if(err)
                {
                    kprint(KPRINT_ERR "    failed to unload module '%s': '%s' depends on this module\n", mod->name, m->name);
                    return -1;
                }
            }
        }
    }

    // Remove module from list
    if(module_list_remove(mod) < 0)
    {
        kprint(KPRINT_ERR "    failed to unload module '%s': unable to remove from list\n", mod->name);
        return -1;
    }

    // Unload module
    if(remove_raw(mod) < 0)
    {
        kprint(KPRINT_ERR "    failed to unload module '%s': internal error\n", mod->name);
        return -1;
    }

    kprint(KPRINT_TRACE "    module '%s' unloaded\n", mod->name);
    return 0;
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////

kmodule* kmodule_insert(const char* name, int load_dependencies)
{
    char* data;
    int size;

    kprint(KPRINT_TRACE "=== loading module '%s'\n", name);

    const char* folder = "/initrd/modules/";
    const char* ext = ".ko";
    char* path = kmalloc(strlen(folder) + strlen(name) + strlen(ext) + 1);
    if(!path)
    {
        kprint(KPRINT_ERR "    failed to load module '%s': kmalloc error", name);
        return 0;
    }

    strcpy(path, folder);
    strcpy(path + strlen(folder), name);
    strcpy(path + strlen(folder) + strlen(name), ext);
    path[strlen(folder) + strlen(name) + strlen(ext)] = '\0';

    struct inode* in = vfs_find(path);
    if(!in)
    {
        kprint(KPRINT_ERR "    failed to load module '%s': file '%s' does not exists\n", name, path);
        return 0;
    }

    if(vfs_rawptr(in, (void**)&data, &size) < 0 || !*data)
    {
        kprint(KPRINT_ERR "    failed to load module '%s': unable to read '%s'\n", name, path);
        return 0;
    }

    kelf* elf = kelf_load(data);
    if(!elf)
    {
        kprint(KPRINT_ERR "    failed to load module '%s': ELF error\n", name);
        return 0;
    }

    kmodule* mod = insert(elf, load_dependencies);

    kprint(KPRINT_TRACE "=== done\n");
    return mod;
}

int kmodule_remove(const char* name, int unload_dependencies)
{
    if(!name)
        return -1;

    kprint(KPRINT_TRACE "=== unloading module '%s'\n", name);

    kmodule* mod = module_list_by_name(name);
    if(!mod)
    {
        kprint(KPRINT_ERR "    failed to unload module '%s': no such module\n", name);
        return -1;
    }

    int ok = remove(mod, unload_dependencies);

    kprint(KPRINT_TRACE "=== done\n");
    return ok;
}
