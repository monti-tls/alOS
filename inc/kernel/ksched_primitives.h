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

#ifndef ALOS_KSCHED_PRIMITIVES_H
#define ALOS_KSCHED_PRIMITIVES_H

#include "platform.h"

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Read the main stack pointer (MSP)
//! \return The msp value
extern uint32_t* read_msp();

//! Read the process stack pointer (PSP)
//! \return The psp value
extern uint32_t* read_psp();

//! Write a value to the process stack pointer (PSP)
//! \param psp The value to write to psp
extern void write_psp(uint32_t* psp);

//! Push the software-saved frame onto the stack, this
//!   helper is used to initialize stacks
extern uint32_t* push_sw_frame(uint32_t* sp);

//! Push the software-saved register frame onto
//!   the stack. This helper is used before
//!   switching stack pointers to save the task's state
extern void ctx_save();

//! Pop the software-saved register frame onto
//!   the stack. This helper is used after
//!   switching stack pointers to restore the task's state
extern void ctx_load();

//! Enter thread mode (by exiting interrupt mode)
extern void thread_mode();

#endif // ALOS_KSCHED_PRIMITIVES_H
