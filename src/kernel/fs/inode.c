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

#include "kernel/fs/inode.h"
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

// N/A

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

// N/A

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

// N/A

/////////////////////////////
//// Public module's API ////
/////////////////////////////

int inode_cdable(struct inode* node)
{
    if (!node)
        return 0;

    return node->tag == I_DIRECTORY;
}

struct inode* inode_find_child(struct inode* root, const char* name)
{
    if (!root || !inode_cdable(root) || !name)
        return 0;

    struct inode* head = root->dir.first;
    while (head)
    {
        if (strcmp(head->name, name) == 0)
            break;

        head = head->next;
    }

    return head;
}

struct inode* inode_parent_dir(struct inode* head, const char* path)
{
    if (!head || !path)
        return 0;

    // Build up a clone of the path to work
    //   with strtok
    int len = strlen(path);
    char* clone = kmalloc(len + 1);
    strcpy(clone, path);

    // Extract first token and loop
    const char* pch = strtok(clone, "/");
    while (pch != NULL)
    {
        // Attempt to cd into the directory
        struct inode* tail = inode_find_child(head, pch);

        // Compute next directory name to go into
        pch = strtok(0, "/");

        // If there is no match, fail if
        //   we've not yet reached the filename
        if (!tail || !pch)
        {
            // Fail if we hit a non-found directory
            if (pch)
                head = 0;

            break;
        }
        else
        {
            // Stop if we reached a file
            if (!inode_cdable(tail))
                break;

            head = tail;
        }
    }

    kfree(clone);

    return head;
}

struct inode* inode_find(struct inode* root, const char* path)
{
    if (!root || !path)
        return 0;

    struct inode* head = inode_parent_dir(root, path);
    if (!head)
        return 0;

    return inode_find_child(head, vfs_filename(path));
}