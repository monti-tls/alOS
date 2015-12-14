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

.syntax unified
.cpu cortex-m4
.thumb
.text

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

.global read_msp
.global read_psp
.global write_psp
.global push_sw_frame
.global ctx_save
.global ctx_load
.global thread_mode

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

read_msp:
	mrs r0, msp
	bx lr

read_psp:
	mrs r0, psp
	bx lr

write_psp:
	msr psp, r0
	bx lr

push_sw_frame:
	stmdb r0!, {r4-r11}
	bx lr

ctx_save:
	mrs r0, psp
	stmdb r0!, {r4-r11}
	msr psp, r0
	bx lr

ctx_load:
	mrs r0, psp
	ldmfd r0!, {r4-r11}
	msr psp, r0
	bx lr

thread_mode:
	ldr pc, =0xFFFFFFFD
