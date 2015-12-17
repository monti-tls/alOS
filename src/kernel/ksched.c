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
#include "drivers/systick.h"
#include "drivers/pendsv.h"

///////////////////////////
//// Module parameters ////
///////////////////////////

//! Size (in words) of the kernel stack
//!   size
#define KERNEL_STACK_SIZE 256

//! Size (in words) of each task's stack
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
static int next_pid();
static int spawn(const char* name, void* start, void* exit, void* arg);
static int schedule();
static void context_switch();
static int rr_init_sched_data(struct ktask* task);
static int rr_schedule(struct ktask* tasks_list, struct ktask** current);
static int h_exit();

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

//! The default, extra simple round-robin scheduling
//!   policy, shipped with this scheduler
static struct ksched_policy rr_policy =
{
    0, // no insert()
    0, // no remove()
    &rr_init_sched_data,
    &rr_schedule
};

//! Contains the current scheduling policy
//! This can be modified at run time using the
//!   appropriate handler
//! If not modified at all, the default round-robin
//!   handler is used
static struct ksched_policy* current_policy = &rr_policy;

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
static void* alloc_stack_page()
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
static int free_stack_page(void* page)
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

//! Find out the next free pid
//! \return The next free pid, -1 if no available or if
//!         other error(s) occured
static int next_pid()
{
    if (!tasks_list)
        return -1;

    for (int pid = 1; pid < MAX_TASKS + 1; ++pid)
    {
        int used = 0;

        for (struct ktask* task = tasks_list->next; task != tasks_list; task = task->next)
        {
            if (!task)
                return -1;

            if (task->pid == pid)
            {
                used = 1;
                break;
            }
        }

        if (!used)
            return pid;
    }

    return -1;
}

//! Spawn a task, that is create it and add it to the
//!   scheduling list
//! \param name Name of the task
//! \param start Start address of the task
//! \param exit Exit handler address of the task
//! \param arg Argument to pass to the task (eventually)
//! \return The pid of the spawned task, -1 if error(s) occured
static int spawn(const char* name, void* start, void* exit, void* arg)
{
    if (!tasks_list || !current_policy)
        return -1;

    int pid = next_pid();
    if (pid < 0)
        return -1;

    struct ktask* task = new_task(pid, name, start, exit, arg);
    if (!task)
        return -1;

    if (tasks_add(task) < 0)
        return -1;

    if (current_policy->init_sched_data(task) < 0)
        return -1;

    return pid;
}

//! Schedule the next task to run and switch to it
//! This uses the scheduling service provided by the current
//!   policy to determine the next task to run
//! It then simply switch tasks and modify the current task pointer
//! \return 0 if all went well, -1 otherwise
static int schedule()
{
    if (!tasks_list || !current_policy)
        return -1;

    // At the very first scheduling event,
    //   no task is ccurrently executed, so
    //   we just setup our stack pointer
    if (!current_task)
    {
        // If we don't have any task to run,
        //   don't do bad stuff 
        if (tasks_list->next == tasks_list)
            return -1;

        // Setup the current task
        current_task = tasks_list->next;

        // Write the stack pointer, the initial
        //   stack frame was crafted in spawn(),
        //   and this context will be loaded
        //   when we have returned in context_switch()
        write_psp(current_task->sp);
    }
    // All other times, we just do normal stuff
    else
    {
        // Determine the next task to run using
        //   the scheduling policy
        struct ktask* next = current_task;
        if (current_policy->schedule(tasks_list, &next) < 0)
            return -1;

        // Switch tasks if needed
        // It's as simple as that because we're currently
        //   in kernel mode (so we use the msp)
        if (next != current_task)
        {
            uint32_t* psp = read_psp();
            write_psp(psp);

            current_task = next;
        }
    }

    return 0;
}

//! Perform a context switch, that is, save the current
//!   task context, call the scheduler (this switched stack
//!   pointers and sets current task) and restore the task context
//! It then returns into the new task in thread mode
//! This function is bound to appropriate interrupt handlers
static void __attribute__((naked)) context_switch()
{
    ctx_save();
    schedule();
    ctx_load();
    thread_mode();
}

//! This is the policy-specific task data initializer
//!   for the default shipped round-robin scheduling policy
//! \param task The task to setup
//! \return 0 if OK, -1 otherwise
static int rr_init_sched_data(struct ktask* task)
{
    if (!task)
        return -1;

    task->sched_data = 0;
    return 0;
}

//! This is the actual scheduling function for the
//!   default shipped round-robin scheduling policy
//! \param tasks_list A pointer to the tasks list
//! \param current Output pointer to the current task,
//!                this function will eventually change
//!                it if a new task was selected for running
static int rr_schedule(struct ktask* tasks_list, struct ktask** current)
{
    if (!tasks_list || !current)
        return -1;

    // If no task was running, we have a problem
    if (!*current)
        return -1;

    // Just loop in the tasks list
    *current = (*current)->next;

    // Remember that the very first task is a dummy one,
    //   so be careful not to set it current
    if (*current == tasks_list)
        *current = (*current)->next;

    return 0;
}

//! This is the default task exit handler
//! \return Does not returns if exiting happened
//!         properly. Otherwise, return -1
static int h_exit()
{
    if (!current_task)
        return -1;

    if (tasks_remove(current_task) < 0)
        return -1;

    // Trigger a PendSV interruption, that will
    //   call context_switch() artificially
    pendsv_trigger();

    return 0;
}

////////////////////////////
//// Interrupt handlers ////
////////////////////////////

//! Systick IRQ handler, alias of our context switch routine
//! This one is used to periodically call the scheduler and
//!   switch between tasks
void __attribute__((alias("context_switch"))) irq_systick_handler();

//! PendSV IRQ handler, alias of our context switch routine
//! This one is used to trigger a schedule immediately (for example
//!   just after a task died)
void __attribute__((alias("context_switch"))) irq_pendsv_handler();

/////////////////////////////
//// Public module's API ////
/////////////////////////////

int ksched_init()
{
    if (tasks_list)
        return -1;

    // Init the stack's layout
    if (init_stack() < 0)
    	return -1;

    // Create and setup the root task
    struct ktask* root = (struct ktask*) kmalloc(sizeof(struct ktask));
    if (!root)
        return -1;

    root->pid = 0;
    root->name = "[root]";
    root->sched_data = 0;
    root->page = 0;
    root->sp = 0;
    root->prev = root->next = root;

    tasks_list = root;

    // Init Systick and PendSV stuff
    systick_init();
    pendsv_init();

    current_task = 0;

    // fork() :
    //  create using new_task(), copying all stuff
    //  register with tasks_add()

    return 0;
}

struct ktask* ksched_task_by_pid(int pid)
{
    if (!tasks_list)
        return 0;

    for (struct ktask* task = tasks_list->next; task != tasks_list; task = task->next)
    {
        if (!task)
            return 0;

        if (task->pid == pid)
            return task;
    }

    return 0;
}

int ksched_change_policy(struct ksched_policy* policy)
{
    if (!policy)
        return -1;

    // Notify old policy about its removal
    if (current_policy->remove)
    {
        if (current_policy->remove() < 0)
            return -1;
    }

    // Switch policies
    current_policy = policy;

    // Notify new policy about its insertion
    if (current_policy->insert)
    {
        if (current_policy->insert() < 0)
            return -1;
    }

    // Reset all tasks' policy-specific data
    for (struct ktask* task = tasks_list->next; task != tasks_list; task = task->next)
    {
        if (task->sched_data)
        {
            kfree(task->sched_data);
            if (current_policy->init_sched_data(task) < 0)
                return -1;
        }
    }

    return 0;
}

int ksched_spawn(const char* name, void* start, void* arg)
{
    if (!tasks_list || !current_policy || !name || !start)
        return -1;

    int pid;
    if ((pid = spawn(name, start, (void*) &h_exit, arg)) < 0)
        return -1;

    // The very first time, start the Systick and
    //   trigger a context switch
    if (!current_task)
    {
        systick_start();
        pendsv_trigger();
    }

    return pid;
}
