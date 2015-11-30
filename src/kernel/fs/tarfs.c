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

//! Enumeration for tar_header.typeflag field.
enum
{
    //! The tar entry is a file
    TF_FILE = '0',
    //! The tar entry is a directory
    TF_DIRECTORY = '5'
};

//! The tar header structure.
typedef struct
{
    //! Path of the file (or directory)
    char path[100];
    //! Chmod stuff (N/U)
    char mode[8];
    //! User id (N/U)
    char uid[8];
    //! Group id (N/U)
    char gid[8];
    //! ASCII-encoded size
    char size[12];
    //! Modification time (N/U)
    char mtime[12];
    //! Checksum (N/U)
    char chksum[8];
    //! Flag designating the type of the entry,
    //!   see TF_*
    char typeflag;
} tar_header;

//! The file data (for file_node.fs_data)
struct file_data
{
    //! Size (in bytes) of the file
    int size;
    //! Pointer to the data in the file
    char* data;
};

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

//! Get the value of an ASCII-encoded size field
//! \param ascii The ASCII-encoded field
//! \return The parsed size, -1 if error(s) occured
static int ascii_size(const char* ascii)
{
    if (!ascii)
        return -1;

    int size = 0;
    int count = 1;

    for (int j = 11; j > 0; j--, count *= 8)
        size += ((ascii[j - 1] - '0') * count);

    return size;
}

//! Get the next header offset from an entry's size
//! \param header The current TAR header
//! \return The offset (in bytes) to the next header, -1
//!         upon failure
static int next_header_offset(tar_header* header)
{
    int size = ascii_size(header->size);
    if (size < 0)
        return -1;

    int off = ((size / 512) + 1) * 512;
    if (size % 512)
        off += 512;
    return off;
}

//! Empty an FS subtree. If node points to a file,
//!   release it, if it points to a directory,
//!   release all its childrens. If the node itself
//!   is released, the pointer *node is set to zero.
//! \param node A pointer to the node to process (set to zero
//!             if the node was released)
//! \return 0 if OK, -1 otherwise
static int empty(struct inode** node)
{
    if (!node || !*node)
        return -1;

    if ((*node)->tag == I_FILE)
    {
        kfree((*node)->file.fs_data);
        kfree(*node);
        *node = 0;
    }
    else if (inode_cdable(*node))
    {
        struct inode* head = (*node)->dir.first;
        while (head)
        {
            struct inode* next = head->next;

            if (empty(&head) < 0)
                return -1;

            if (head)
                kfree(head);

            head = next;
        }

        (*node)->dir.first = (*node)->dir.last = 0;
    }

    return 0;
}

//! Unmount and release a tarfs
//! \param root The root node of the FS to release
//! \return 0 if all went well, -1 otherwise
static int o_umount(struct inode* root)
{
    if (!root)
        return -1;

    if (empty(&root) < 0)
        return -1;

    kfree(root->superblock);

    return 0;
}

//! Get a raw pointer to a file's data buffer
//! \param node The inode
//! \param ptr Output parameter to the inode's data buffer
//! \param size Output parameter for the inode's data buffer size
//! \return 0 if OK, -1 otherwise
static int o_rawptr(struct inode* node, void** ptr, int* size)
{
    if (!node || node->tag != I_FILE || !ptr || !size)
        return -1;

    struct file_data* file = (struct file_data*)node->file.fs_data;
    if (!file)
        return -1;

    *ptr = file->data;
    *size = file->size;

    return 0;
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////

int tarfs_mount(struct inode* root, void* tarblob)
{
    if (!root || !tarblob)
        return -1;

    // Configure the fs' superblock
    struct superblock* super = kmalloc(sizeof(struct superblock));

    super->fs_name = "tarfs";
    super->flags = FSF_RDONLY | FSF_RAM;
    super->umount = &o_umount;
    super->mkdir = 0;
    super->rawptr = &o_rawptr;

    root->superblock = super;

    // First pass, create directories inodes
    int offset = 0;
    for (int i = 0;; i++)
    {
        // Get the current header, stop at end
        tar_header* header = (tar_header*)(tarblob + offset);
        if (*header->path == '\0')
            break;

        // We only care about directories for now
        if (header->typeflag == TF_DIRECTORY)
        {
            // Get the parent directory if the current one,
            //   this approach works because GNU tar puts
            //   nested directories in order, for example :
            // test/
            // test/a
            // test/b
            // test/b/c
            struct inode* parent = inode_parent_dir(root, header->path);
            if (!parent)
            {
                return -1;
            }

            // Create the current directory's inode, don't
            //   forget to patch its filename because it ends
            //    with a slash.
            struct inode* dir = kmalloc(sizeof(struct inode));
            dir->superblock = super;
            dir->tag = I_DIRECTORY;
            dir->name = (char*)vfs_filename(header->path);
            vfs_path_clean(dir->name);

            dir->next = 0;
            dir->dir.first = dir->dir.last = 0;

            // Insert this inode in its parent's list
            if (!parent->dir.first)
                parent->dir.first = parent->dir.last = dir;
            else
                parent->dir.last = (parent->dir.last->next = dir);
        }

        offset += next_header_offset(header);
    }

    // Second pass, register files
    offset = 0;
    for (int i = 0;; i++)
    {
        // Get the current header, stop at end
        tar_header* header = (tar_header*)(tarblob + offset);
        if (*header->path == '\0')
            break;

        if (header->typeflag == TF_FILE)
        {
            // Get the inode's parent directory, now
            //   that they are created
            struct inode* parent = inode_parent_dir(root, header->path);
            if (!parent)
            {
                return -1;
            }

            // Create the file's inode
            struct inode* file = kmalloc(sizeof(struct inode));
            file->superblock = super;
            file->tag = I_FILE;
            file->name = (char*)vfs_filename(header->path);
            file->next = 0;

            struct file_data* fs_data = (struct file_data*)kmalloc(sizeof(struct file_data));
            fs_data->size = ascii_size(header->size);
            fs_data->data = (char*)(tarblob + offset + 512);
            file->file.fs_data = fs_data;

            // Insert the inode in its parent's linked list
            if (!parent->dir.first)
                parent->dir.first = parent->dir.last = file;
            else
                parent->dir.last = (parent->dir.last->next = file);
        }

        offset += next_header_offset(header);
    }

    return 0;
}
