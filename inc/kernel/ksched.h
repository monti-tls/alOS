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

#ifndef ALOS_KSCHED_H
#define ALOS_KSCHED_H

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Initialize the kernel scheduler, do not launch any
//!   task for now.
//! \return 0 if scheduler is initialized OK, -1 otherwise
int ksched_init();

#endif // ALOS_KSCHED_H
