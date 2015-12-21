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

.extern ksysmap
.extern ksysmap_size

.global ksyscall_init
.global irq_svc_handler

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

// N/A

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

// N/A

////////////////////////////
//// Interrupt handlers ////
////////////////////////////

//! This is the IRQ handler for the service call
//!   interrupt, that is triggered by the 'svc' instruction
.type  irq_svc_handler, %function
irq_svc_handler:
	push {lr} // save the return from interrupt code
	cpsid i // enter critical section

	// Read in r0 the syscall id from the
	//   svc instruction
    mrs r0, psp
    add r0, #24 // saved PC address in hardware context
    ldr r0, [r0] // read saved PC value
    sub r0, #2 // get the svc instruction address
    ldr r0, [r0] // get instruction value
    mov r1, #0xFF
    and r0, r0, r1 // clear the unused bytes

    // Read the syscall arguments from the saved context
    // In r1-r3
    mrs r4, psp
    ldmia r4, {r1-r3}

    // Branch to the appropriate syscall handler
    // This function is defined in ksysmap.c/h
    bl ksysmap_jump

    // Write the return value in the saved context
    // mrs r1, psp
    // str r0, [r1]

    cpsie i // exit critical section
    pop {pc} // return from exception

/////////////////////////////
//// Public module's API ////
/////////////////////////////

ksyscall_init:
	push {lr}
	bl svcall_init
    mov r0, #0
	pop {pc}
