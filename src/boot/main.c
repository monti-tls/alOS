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

#include "platform.h"
#include "kernel/kprint.h"
#include "kernel/ksymbols.h"
#include "kernel/kmalloc.h"
#include "kernel/kelf.h"
#include "kernel/kmodule.h"

#include "kernel/fs/inode.h"
#include "kernel/fs/vfs.h"
#include "kernel/fs/tarfs.h"

#include <string.h>

void print_initrd(struct inode* node, int indent)
{
    kprint(KPRINT_TRACE);
    for(int i = 0; i < indent; ++i)
        kprint(" ");
    kprint(node->name);
    kprint("\n");

    if(inode_cdable(node))
    {
        struct inode* head = node->dir.first;
        while(head)
        {
            print_initrd(head, indent + 2);
            head = head->next;
        }
    }
}

void export_ksymbols()
{
    // ksymbols.h exports
    ksymbol_add("ksymbol_add", &ksymbol_add);
    ksymbol_add("ksymbol", &ksymbol);

    // kprint.h exports
    ksymbol_add("kprint", &kprint);

    // kmalloc.h exports
    ksymbol_add("kmalloc", &kmalloc);
    ksymbol_add("krealloc", &krealloc);
    ksymbol_add("kfree", &kfree);

    // kelf.h exports
    ksymbol_add("kelf_load", &kelf_load);
    ksymbol_add("kelf_unload", &kelf_unload);
    ksymbol_add("kelf_symbol", &kelf_symbol);

    // fs/inode.h exports
    ksymbol_add("inode_cdable", &inode_cdable);
    ksymbol_add("inode_find_child", &inode_find_child);
    ksymbol_add("inode_parent_dir", &inode_parent_dir);
    ksymbol_add("inode_find", &inode_find);

    // fs/tarfs.h exports
    ksymbol_add("tarfs_mount", &tarfs_mount);

    // fs/vfs.h exports
    ksymbol_add("vfs_filename", &vfs_filename);
    ksymbol_add("vfs_path_clean", &vfs_path_clean);
    ksymbol_add("vfs_find", &vfs_find);
    ksymbol_add("vfs_umount", &vfs_umount);
    ksymbol_add("vfs_mkdir", &vfs_mkdir);
    ksymbol_add("vfs_rawptr", &vfs_rawptr);
}

int main()
{
    int err;

    // Init the SWO debug module
    kprint_init();

    // Init memory allocation
    err = kmalloc_init();
    if(err < 0)
        kprint(KPRINT_ERR "kmalloc_init() failed\n");
    else
        kprint(KPRINT_MSG "memory allocator initialized\n");

    // Export kernel symbols
    export_ksymbols();
    kprint(KPRINT_MSG "exported kernel symbols\n");

    // Get root inode
    struct inode* root_in = vfs_find("/");

    // Mkdir initrd mount point
    err = vfs_mkdir(root_in, "initrd");
    struct inode* initrd_in = vfs_find("/initrd");
    if(err < 0 || !initrd_in)
        kprint(KPRINT_ERR "unable to mkdir initrd mount point\n");
    else
        kprint(KPRINT_MSG "initrd mount point created\n");

    // Mount the initrd filesystem
    extern int _ld_initrd_start;
    err = tarfs_mount(initrd_in, &_ld_initrd_start);
    if(err < 0)
        kprint(KPRINT_ERR "unable to mount initrd\n");
    else
        kprint(KPRINT_MSG "initrd mounted\n");

    // Log its contents
    kprint(KPRINT_TRACE "=== initrd dump ===\n");
    print_initrd(initrd_in, 0);
    kprint(KPRINT_TRACE "===================\n");

    // Load the module 'sample'
    kmodule_insert("a", 1);

    // Unload the module he depends on
    kmodule_remove("b", 1);

    // Unmount the FS
    if(vfs_umount(root_in) < 0)
        kprint(KPRINT_ERR "unable to unmount rootfs\n");
    else
        kprint(KPRINT_MSG "unmounted rootfs\n");

    return 0;
}
