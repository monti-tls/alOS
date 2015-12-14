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

#include "kernel/ksched.h"
#include "kernel/ksched_primitives.h"
#include "kernel/kmalloc.h"
#include "platform.h"

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

	//! Address of the saved stack pointer of the task
	uint32_t* sp;

	//! Pointer to the previous task in the doubly
	//!   linked list
	struct ktask* prev;
	//! Pointer to the next task in the doubly linked list
	struct ktask* next;
};

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

static int tasks_add(struct ktask* task);
static int tasks_remove(struct ktask* task);

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

//! All active tasks are stored in a doubly
//!   (cyclic) linked list
//! It always contains at lease a task, which is
//!   initialized at startup and holds an empty (pid 0) task
static struct ktask* tasks_list = 0;

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

//! Add a task in the linked list
//! \param task The task to add in the list
//! \return 0 if OK, -1 otherwise
static int tasks_add(struct ktask* task)
{
	if (!tasks_list || !task)
		return -1;

	// Get the last task in the list (just before the first,
	//   as the list is circular)
	struct ktask* last = tasks_list->prev;

	// This works even when there's only one task
	//   in the list, as last == tasks_list in this case
	last->next = task;
	task->next = tasks_list;
	tasks_list->prev = task;
	task->prev = last;

	return 0;
}

//! Remove a task from the linked list. This
//!   *does* free the task
//! \param task The task to remove
//! \return 0 if OK, -1 otherwise
static int tasks_remove(struct ktask* task)
{
	if (!task || !task->next || !task->prev)
		return -1;

	task->prev->next = task->next;
	task->next->prev = task->prev;

	task->next = 0;
	task->prev = 0;

	if (task->sched_data)
		kfree(task->sched_data);
	kfree(task);

	return 0;
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////
