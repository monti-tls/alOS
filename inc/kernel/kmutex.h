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

#ifndef ALOS_KMUTEX_H
#define ALOS_KMUTEX_H

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

//! Type of a kernel mutex
typedef unsigned int kmutex;

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Lock a mutex (blocking call)
//! \param mutex The mutex to lock
void kmutex_lock(kmutex* mutex);

//! Unlock a mutex
//! \param mutex The mutex to unlock
void kmutex_unlock(kmutex* mutex);

#endif // ALOS_KMUTEX_H
