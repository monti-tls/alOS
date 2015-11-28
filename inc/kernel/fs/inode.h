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

#ifndef ALOS_INODE_H
#define ALOS_INODE_H

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

struct inode;
struct dir_node;
struct file_node;

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

//! Enumeration for the superblock_node.flags field
enum
{
    //!< The filesystem is read-only
    FSF_RDONLY = 0x0001,
    //!< The filesystem physically resides in the ram
    FSF_RAM = 0x0002
};

//! A superblock node, used in the VFS to identify
//!   the filesystem implementation to which an inode
//!   subtree belongs to.
struct superblock
{
    //!< Name of the filesystem
    const char* fs_name;
    //!< Flags of the filesystem, from FSF_*
    int flags;

    //! Below are function pointers to the filesystem's
    //!   available operations.

    //!< Unmount the filesystem from this node
    //!< Releases all child inodes, but *not* the mount point
    //!< Also releases the associated superblock
    //!< (MANDATORY)
    int (*umount)(struct inode*);

    //!< Create a directory in the given inode
    //!< (unless FSF_READONLY)
    int (*mkdir)(struct inode*, const char*);

    //!< Get a raw pointer to the file's data
    //!< (FSF_RAM only)
    int (*rawptr)(struct inode*, void**, int*);

    // @TODO: more operations here
};

//! A directory node structure.
struct dir_node
{
    //!< Pointer to the first inode in the list
    struct inode* first;
    //!< Pointer to the last inode in the list
    struct inode* last;
};

//! A file node structure.
struct file_node
{
    //! Filesystems will put their own data
    //!   structure here.
    void* fs_data;
};

//! Inode's type tag enumeration.
enum
{
    //!< The inode is a simple file
    I_FILE = 0x01,
    //!< The inode is a directory
    I_DIRECTORY = 0x02
};

//! The inode structure
struct inode
{
    //!< Type tag, from I_*
    int tag;
    //!< The name of the inode (ASCII,
    //!<   null-terminated)
    char* name;

    //!< An union of the different possible
    //!<   node values (deduced from the type tag)
    union
    {
        struct dir_node dir;
        struct file_node file;
    };

    //!< Pointer to the next inode in the same scope
    struct inode* next;

    //!< A pointer to this inode's superblock
    //!< (must be set for all inodes)
    struct superblock* superblock;
};

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Says wether or not an inode contains other inodes.
//! It is the case for directories and superblocks.
//! \param node The node to examine
//! \return 1 if the node contains other inodes, 0 otherwise
int inode_cdable(struct inode* node);

//! Find an inode by name in a directory inode.
//! \param root The directory inode to search into
//! \param name The name of the inode to find
//! \return A pointer to the found inode, 0 if not found
struct inode* inode_find_child(struct inode* root, const char* name);

//! Search for the parent directory's inode of a path
//! For example, if called with a/b/c (and if a and b exists),
//!   this function will return the inode 'c'.
//! \param head The inode to search in (must be a directory)
//! \param path The path of the file
//! \return A pointer to the parent inode, 0 if not found
struct inode* inode_parent_dir(struct inode* head, const char* path);

//! Find (recursively) a file (or directory)'s inode.
//! \param root The root inode to search into
//! \param path The path of the file to find (*without* trailing space)
//! \return The found inode, 0 if not found
struct inode* inode_find(struct inode* root, const char* path);

#endif // ALOS_INODE_H
