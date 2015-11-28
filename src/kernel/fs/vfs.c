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

#include "kernel/fs/vfs.h"
#include "kernel/kmalloc.h"
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

// N/A

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

static int o_umount(struct inode* root);
static int o_mkdir(struct inode* node, const char* name);

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

//! The VFS superblock (because VFS has its own operations).
static struct superblock _superblock = {
    "vfs",
    0,
    &o_umount,
    &o_mkdir,
    0 // rawptr
};
static struct superblock* superblock = &_superblock;

//! The root FS inode.
static struct inode _root = {I_DIRECTORY, "/", {{0, 0}}, 0, &_superblock};

static struct inode* root = &_root;

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

//! \return 0 if OK, -1 otherwise
static int empty(struct inode** node)
{
    if(!node || !*node || !inode_cdable(*node))
        return -1;

    // For VFS-owned inodes, just delete them normally
    if((*node)->superblock == superblock && inode_cdable(*node))
    {
        struct inode* head = (*node)->dir.first;
        while(head)
        {
            struct inode* next = head->next;

            if(empty(&head) < 0)
                return -1;

            if(head)
            {
                kfree(head->name);
                kfree(head);
            }

            head = next;
        }

        (*node)->dir.first = (*node)->dir.last = 0;
    }
    // For other FS, unmount them properly before deleting
    else
    {
        if((*node)->superblock->umount(*node) < 0)
            return -1;
    }

    return 0;
}

//! Unmount and release the VFS
//! \param root The root node of the FS to release
//! \return 0 if all went well, -1 otherwise
static int o_umount(struct inode* root)
{
    if(!root)
        return -1;

    if(empty(&root) < 0)
        return -1;

    return 0;
}

static int o_mkdir(struct inode* node, const char* name)
{
    if(!node || !inode_cdable(node) || !name)
        return -1;

    // Don't create inodes with same names
    if(inode_find_child(node, name))
        return -1;

    struct inode* dir = kmalloc(sizeof(struct inode));
    dir->superblock = superblock;
    dir->tag = I_DIRECTORY;

    int len = strlen(name);
    dir->name = kmalloc(len + 1);
    strcpy(dir->name, name);
    vfs_path_clean(dir->name);

    dir->next = 0;
    dir->dir.first = dir->dir.last = 0;

    // Insert this inode in its parent's list
    if(!node->dir.first)
        node->dir.first = node->dir.last = dir;
    else
        node->dir.last = (node->dir.last->next = dir);

    return 0;
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////

const char* vfs_filename(const char* name)
{
    int len = strlen(name);
    char* clone = kmalloc(len + 1);
    strcpy(clone, name);

    const char* last;
    const char* pch = strtok(clone, "/");
    while(pch != NULL)
    {
        last = pch;
        pch = strtok(0, "/");
    }

    int off = (int)(last - clone);

    kfree(clone);

    return name + off;
}

int vfs_path_clean(char* name)
{
    if(!name)
        return -1;

    int len = strlen(name);
    if(name[len - 1] == '/')
        name[len - 1] = '\0';

    return 0;
}

struct inode* vfs_find(const char* path)
{
    if(!path)
        return 0;

    if(strcmp(path, "/") == 0)
        return root;

    return inode_find(root, path);
}

int vfs_umount(struct inode* root)
{
    if(!root || !root->superblock)
        return -1;

    if(!root->superblock->umount)
        return -1;

    return root->superblock->umount(root);
}

int vfs_mkdir(struct inode* node, const char* name)
{
    if(!node || !name || !node->superblock)
        return -1;

    if(!node->superblock->mkdir)
        return -1;

    return node->superblock->mkdir(node, name);
}

int vfs_rawptr(struct inode* node, void** ptr, int* size)
{
    if(!node || !ptr || !size || !node->superblock)
        return -1;

    if(!node->superblock->rawptr)
        return -1;

    return node->superblock->rawptr(node, ptr, size);
}
