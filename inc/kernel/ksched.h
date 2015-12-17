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

#include "platform.h"

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

struct ktask;
struct ksched_policy;

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

//! The kernel structure representing a
//!   task.
struct ktask
{
    //! Process identifier (unique) of the task
    int pid;
    //! Textual name of the task
    const char* name;
    //! Scheduler-specific data, must
    //!   point to something that does not contains
    //!   allocated memory inside, i.e. that can be
    //!   freed without seeing what's inside
    void* sched_data;

    //! Address of this task's stack page
    void* page;
    //! Address of the saved stack pointer of the task
    void* sp;

    //! Pointer to the previous task in the doubly
    //!   linked list
    struct ktask* prev;
    //! Pointer to the next task in the doubly linked list
    struct ktask* next;
};

//! Structure holding specific scheduler policy
//!   code. Provides basic functions to implement
//!   particular scheduling policies using
//!   this module's services.
struct ksched_policy
{
    //! This function is called (if not null) when the scheduling
    //!   policy is inserted into the system (use it to register
    //!   symbols for example)
    int (*insert)();
    
    //! This function is called (if not null) when the scheduling
    //!   policy is removed froom the system (use it to un-register
    //!   symbols for example)
    int (*remove)();

    //! Callback for the scheduler-specific data initialization
    //!   (ktask.sched_data member), this data is de-initialized
    //!   automatically by this module
    int (*init_sched_data)(struct ktask*);

    //! Callback for the scheduling work, first argument
    //!  is the first element of the tasks list, the second
    //!  one is a pointer to the current task, and must be set
    //!  before returning from this function to the scheduled task
    int (*schedule)(struct ktask*, struct ktask**);
};

/////////////////////////////
//// Public module's API ////
/////////////////////////////

//! Initialize the kernel scheduler, do not launch any
//!   task for now.
//! The scheduling policy is initially set to the default one, a very
//!   simple round-robin scheduler
//! \return 0 if scheduler is initialized OK, -1 otherwise
int ksched_init();

//! Retrieve a task given its pid
//!   (linear in time)
//! \param pid The pid to search
//! \return The found task, 0 if not found
struct ktask* ksched_task_by_pid(int pid);

//! Change the current scheduling policy
//! This resets any policy-specific data in all tasks,
//!   resetting them to the default values using
//!   ksched_policy.init_sched_data
//! \param policy The new scheduling policy to adopt
//! \return 0 if all went well, -1 otherwise
int ksched_change_policy(struct ksched_policy* policy);

//! Spawn a task
//! This is used as a basic service by kernel threads
//!   and user program loading
//! The first call to this function will start the scheduler,
//!   and therefore never return (if succesful)
//! \param name ASCII string containing the name of the task,
//!             must not be allocated
//! \param start Start address to jump to when starting the task
//! \param An eventual argument to pass to the task
//! \return The pid (> 1) of the spawned task if OK, -1 otherwise
int ksched_spawn(const char* name, void* start, void* arg);

#endif // ALOS_KSCHED_H
