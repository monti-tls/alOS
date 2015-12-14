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

///////////////////////////
//// Module parameters ////
///////////////////////////

//! Size (in words) of the kernel stack's
//!   size
#define KERNEL_STACK_SIZE 256

//! Size (in words) of each task's stack's
//!   size
#define TASK_STACK_SIZE 128

//! Base address of the stack
#define STACK_BASE ((void*) 0x20000000)

//! Size of the stack (in bytes)
#define STACK_SIZE 0x00020000

//! Size (in words) of the hardware
//!  pushed frame
#define HW_FRAME_SIZE 8

//! Maximum number of tasks in the system
//!  (ram size dependent)
#define MAX_TASKS (((STACK_SIZE / 4) - KERNEL_STACK_SIZE) / TASK_STACK_SIZE)

////////////////////////////////
//// Module's sanity checks ////
////////////////////////////////

#if (MAX_TASKS <= 3)
# error "Not enough space for such big stack pages"
#endif

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

//! Stack page flags. Holded at the first
//!   word of each stack page, those flags
//!   are used to store their status for
//!   stack page allocation
enum
{
	//! The page is currently being used by a task
	SPF_USED = 0x01
};

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

static int tasks_add(struct ktask* task);
static int tasks_remove(struct ktask* task);
static struct ktask* new_task(int pid, const char* name, void* start, void* exit, void* arg);
static int init_stack();
static void* alloc_stack_page();
static int free_stack_page(void*);

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

//! Address of the very end of the stack
static uint32_t* stack_top = (uint32_t*) (STACK_BASE + STACK_SIZE);

//! All active tasks are stored in a doubly
//!   (cyclic) linked list
//! It always contains at lease a task, which is
//!   initialized at startup and holds an empty (pid 0) task
static struct ktask* tasks_list = 0;

//! Points to the currently executed task.
//! This is updated when all has been initialized
//!   correctly, and is potentially modified after
//!   each scheduling interrupt
static struct ktask* current_task = 0;

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

//! Create a new valid task structure, but do
//!   *not* insert it into the list, neither initialize
//!   its scheduler-specific data
//! \param pid The pid of the task to create
//! \param name The name of the task to create
//! \param start The start address of the task's executable code
//!              (i.e. entry point for the task)
//! \param exit Exit point of the task (must point to task deletion code)
//! \param arg Argument to pass to the task
//! \return The created task
static struct ktask* new_task(int pid, const char* name, void* start, void* exit, void* arg)
{
	struct ktask* task = (struct ktask*) kmalloc(sizeof(struct ktask));
	if (!task)
		return 0;

	// Initialize the task's data
	task->pid = pid;
	task->name = name;
	task->sched_data = 0;
	task->next = task->prev = 0;

	// Get some stack space
	task->page = alloc_stack_page();
	if (!task->page)
	{
		kfree(task);
		return 0;
	}

	// Craft the initial stack frame
	uint32_t* sp = ((uint32_t*) task->page) - HW_FRAME_SIZE + 1;
	sp[7] = 0x21000000; // xPSR
	sp[6] = (uint32_t) start; // PC
	sp[5] = (uint32_t) exit; // LR
	sp[4] = 0; // R12
	sp[3] = 0; // R3
	sp[2] = 0; // R2
	sp[1] = 0; // R1
	sp[0] = (uint32_t) arg; // R0

	task->sp = (void*) push_sw_frame(sp);

	return task;
}

//! Init the stack layout (must be called
//!   before any task is created)
//! \return 0 if OK, -1 otherwise
static int init_stack()
{
	// Just clear out all status flags
	for (int i = 0; i < MAX_TASKS; ++i)
	{
		uint32_t* flag = stack_top - KERNEL_STACK_SIZE - i * TASK_STACK_SIZE;
		*flag = 0;
	}

	return 0;
}

//! Request the allocation of a stack page.
//! \return The usable address of a stack page,
//!         *not* the starting address of the page
//!         (as some data is stored at the beginning)
void* alloc_stack_page()
{
	for (int i = 0; i < MAX_TASKS; ++i)
	{
		uint32_t* flag = stack_top - KERNEL_STACK_SIZE - i * TASK_STACK_SIZE;

		if (!(*flag & SPF_USED))
			return flag - sizeof(uint32_t);
	}

	return 0;
}

//! Release a stack page
//! \param The stack page *usable* address
//! \return 0 if OK, -1 otherwise
int free_stack_page(void* page)
{
	if (!page)
		return -1;

	// Fix the page address to go to the status flag
	uint32_t* flag = ((uint32_t*) page) + 1;

	if (*flag & SPF_USED)
		*flag ^= SPF_USED;
	else
		return -1;

	return 0;
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////

int ksched_init()
{
    if (tasks_list)
        return -1;

    // Init the stack's layout
    if (!init_stack())
    	return -1;

    // Create and setup the root task
    struct ktask* root = (struct ktask*)kmalloc(sizeof(struct ktask));
    if (!root)
        return -1;

    root->pid = 0;
    root->name = "[root]";
    root->sched_data = 0;
    root->page = 0;
    root->sp = 0;
    root->prev = root->next = 0;

    tasks_list = root;

    // init() :
    //   spawn init task, schedule()
    //
    // spawn() :
    //   create task using new_task()
    //   register with tasks_add()
    //
    // fork() :
    //  create using new_task(), copying all stuff
    //  register with tasks_add()
    //
    // schedule() : (called with Systick plus PendSV)
    //   call policy->schedule()
    //   update ?

    return 0;
}
