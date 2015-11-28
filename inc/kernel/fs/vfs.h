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

#ifndef ALOS_VFS_H
#define ALOS_VFS_H

#include "inode.h"
#include "tarfs.h"
#include "vfs.h"

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Get the file name part of an URL
//! \param name An URL of the form a/b/.../c
//! \return The file name part (e.g 'c'), with the
//!         trailing slash if present. The returned
//!         pointer is just an offset from name and is
//!         not from allocated memory
const char* vfs_filename(const char* name);

//! Clean a path (removes the eventual trailing slash)
//! \param name The filename to patch
//! \return 0 if OK, -1 otherwise
int vfs_path_clean(char* name);

//! Find an inode by path
//! \param path The path of the inode to retrieve
//! \return The inode, 0 if not found
struct inode* vfs_find(const char* path);

//! Unmount a filesystem and release all its resources
//! \param node The filesystem's root node
//! \return 0 if all went well, -1 otherwise
int vfs_umount(struct inode* root);

//! Create a directory entry
//! \param node The inode to create the directory into
//! \return 0 if all went well, -1 otherwise
int vfs_mkdir(struct inode* node, const char* name);

//! Get a raw pointer to a file's data buffer
//! \param node The inode
//! \param ptr Output parameter to the inode's data buffer
//! \param size Output parameter for the inode's data buffer size
//! \return 0 if OK, -1 otherwise
int vfs_rawptr(struct inode* node, void** ptr, int* size);

#endif // ALOS_VFS_H
