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

.equ locked, 1
.equ unlocked, 0

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

.global kmutex_lock
.global kmutex_unlock

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

// see http://infocenter.arm.com/help/topic/com.arm.doc.dht0008a/DHT0008A_arm_synchronization_primitives.pdf
// for reference

kmutex_lock:
    ldr     r1, =locked
_1:
    ldrex   r2, [r0]     // read mutex
    cmp     r2, r1
    beq     _2           // if locked, go again
    strex   r2, r1, [r0] // if not, try to lock
    cmp     r2, #1       // check if suceeded
    beq     _1           // if not, go again
    dmb                  // memory barrier
    bx      lr
_2:
    // place wait for update here
    b _1

kmutex_unlock:
    ldr     r1, =unlocked
    dmb                  // memory barrier
    str     r1, [r0]     // just write to address
    bx      lr
