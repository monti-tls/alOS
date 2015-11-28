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

#include "kernel/kmalloc.h"

///////////////////////////
//// Module parameters ////
///////////////////////////

// KMALLOC_POOL_SIZE is defined at compile time
#define POOL_SIZE KMALLOC_POOL_SIZE
// KMALLOC_POOL_DEPTH is defined at compile time
#define DEPTH KMALLOC_POOL_DEPTH

////////////////////////////////
//// Module's sanity checks ////
////////////////////////////////

#if(POOL_SIZE >> DEPTH) == 0
#error "Depth is too important for the selected pool size"
#endif

#if((POOL_SIZE >> DEPTH) % KMALLOC_ALIGNMENT) != 0
#error "Depth does not guarantee alignment"
#endif

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

// As we have 2^o blocks for all orders between 0 and DEPTH-1, we have
//   in total 2^depth - 1 blocks
#define BLOCKS_COUNT ((1 << DEPTH) - 1)

//! An enumeration for blocks statuses
enum {
    //!< The block is free to use
    F_FREE = 0x00,
    //!< The block is used
    F_USED = 0x01,
    //!< The block is unusable because it is part
    //!<   of a bigger used block
    F_BLOCKED_BY_PARENT = 0x02,
    //!< The block is unusable because some of
    //!<   its child blocks are used
    F_BLOCKED_BY_CHILD = 0x04
};

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

//! Array of all elementary blocks sizes, by order.
static int blocks_size[DEPTH];
//! Array of all elementary block counts, by order.
static int blocks_count[DEPTH];
//! Array of all block statuses, by global block id.
static int blocks_statuses[BLOCKS_COUNT];

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

//! Compute a global block id from its order and
//!   identifier.
//! \param order The order of the block
//! \param id The id of the block (in its order)
//! \return The global id of the block
static int block_id(int order, int id) {
    if(order < 0 || order >= DEPTH)
        return -1;
    if(id < 0 || id >= blocks_count[order])
        return -1;

    // Order offset is sum (i=0 -> order-1, 2^i) = 2^order - 1
    return id + ((1 << order) - 1);
}

//! Get a pointer to a given block's status
//! \param order The order of the block
//! \param id The id of the block (in its order)
//! \return 0 if invalid order and/or id, a pointer
//!         to the block's status otherwise
static int* block_status(int order, int id) {
    int glob = block_id(order, id);
    if(glob < 0)
        return 0;

    return blocks_statuses + glob;
}

//! Set the status of all parents of a given block
//! \param order The order of the block
//! \param id The id of the block (in its order)
//! \param status The value to write in the blocks' statuses
//! \return 0 on success, -1 on failure
static int mark_parents(int order, int id, int status) {
    while(order != 0) {
        order -= 1;
        id >>= 1;

        int* s = block_status(order, id);
        if(!s)
            return -1;

        if(*s == status)
            break;

        *s = status;
    }

    return 0;
}

//! Set the status of all children of a given block
//! \param order The order of the block
//! \param id The id of the block (in its order)
//! \return 0 on success, -1 on failure
static int mark_children(int order, int id, int status) {
    order += 1;
    id <<= 1;

    if(order >= DEPTH)
        return 0;

    for(int i = 0; i < 2; ++i) {
        int* s = block_status(order, id + i);
        if(!s)
            return -1;

        if(*s != status) {
            *s = status;
            if(mark_children(order, id + i, status) < 0)
                return -1;
        }
    }

    return 0;
}

//! Find a used block by its offset.
//! \param offset The offset of the block (for example returned by alloc)
//! \param order Output parameter for the found block order
//! \param id Output parameter for the found block id
//! \param status Output parameter for the found block status
static void find_used(int offset, int* order, int* id, int** status) {
    for(int o = 0; o < DEPTH; ++o) {
        int i = offset / blocks_size[o];

        *status = block_status(o, i);
        if(!*status)
            return;

        if(**status == F_USED) {
            *order = o;
            *id = i;
            break;
        }
    }
}

//! Allocate a block of memory.
//! \param size The size of the block to allocate, in bytes
//! \return The offset of the alloc'ed block in bytes, or -1 on failure
static int alloc(int size) {
    if(size <= 0 || size > POOL_SIZE)
        return -1;

    // Seek for the associated order
    int order = -1;
    for(int o = DEPTH - 1; o >= 0; --o) {
        if(size <= blocks_size[o]) {
            order = o;
            break;
        }
    }

    // Normally never reached due to the check above
    if(order < 0)
        return -1;

    // Search the first free block
    int id = -1;
    int* s;
    for(int i = 0; i < blocks_count[order]; ++i) {
        s = block_status(order, i);
        if(!s)
            return -1;

        if(*s == F_FREE) {
            id = i;
            break;
        }
    }

    // There is not enough space for this block size
    if(id < 0)
        return -1;

    // Tag this block
    *s = F_USED;

    // Tag parent and children blocks
    if(mark_children(order, id, F_BLOCKED_BY_PARENT) < 0)
        return -1;
    if(mark_parents(order, id, F_BLOCKED_BY_CHILD) < 0)
        return -1;

    return id * blocks_size[order];
}

//! Release a block of memory.
//! \param offset The base offset of the block of memory to release
//! \return 0 on success, -1 upon failure
static int release(int offset) {
    if(offset < 0 || offset >= POOL_SIZE)
        return -1;

    // Seek for the used block mapped to this offset
    int order = -1;
    int id = -1;
    int* s;
    find_used(offset, &order, &id, &s);

    // We never alloc'ed this block !
    if(order < 0 || id < 0)
        return -1;

    // Tag this block as free
    *s = F_FREE;

    // Tag children as free
    if(mark_children(order, id, F_FREE) < 0)
        return -1;

    // Coalesce blocks
    for(;;) {
        // Read my buddy's status
        int buddy = id ^ 1;
        int* s = block_status(order, buddy);
        if(!s)
            return -1;

        // If my buddy is free, we can free our parent
        if(*s == F_FREE) {
            order = order - 1;
            id = id / 2;

            s = block_status(order, id);
            if(!s)
                return -1;

            *s = F_FREE;

            if(order <= 0)
                break;
        } else
            break;
    }

    return 0;
}

//! Init the buddy allocator's internal variables.
static void init() {
    // Compute block sizes
    for(int o = 0; o < DEPTH; ++o)
        blocks_size[o] = POOL_SIZE >> o;

    // Compute block counts
    for(int o = 0; o < DEPTH; ++o)
        blocks_count[o] = 1 << o;

    // Tag all blocks as free
    for(int o = 0; o < DEPTH; ++o) {
        for(int i = 0; i < blocks_count[o]; ++i) {
            int id = block_id(o, i);
            blocks_statuses[id] = F_FREE;
        }
    }
}

//! Print out the allocator's state.
static void __attribute__((unused)) dump(void (*debug)(const char*, ...)) {
    for(int o = 0; o < DEPTH; ++o) {
        (*debug)("%2d [%04d]: ", o, blocks_size[o]);

        for(int i = 0; i < blocks_count[o]; ++i) {
            int spaces = (1 << (DEPTH - 1 - o)) - 1;
            if(i == 0)
                spaces /= 2;
            if(spaces)
                (*debug)("%*s", spaces, " ");

            int* s = block_status(o, i);

            if(*s == F_FREE)
                (*debug)("F");
            else if(*s == F_USED)
                (*debug)("U");
            else if(*s == F_BLOCKED_BY_CHILD)
                (*debug)("C");
            else if(*s == F_BLOCKED_BY_PARENT)
                (*debug)("P");
        }

        (*debug)("\n");
    }
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////

extern int _ld_kmalloc_start;
static void* kmalloc_pool = (void*)&_ld_kmalloc_start;

int kmalloc_init() {
    init();

    return 0;
}

void* kmalloc(int size) {
    int offset = alloc(size);
    if(offset < 0)
        return 0;

    return kmalloc_pool + offset;
}

void* krealloc(void* ptr, int size) {
    if(!size)
        return 0;

    // Try to allocate the new buffer
    void* new_buf = kmalloc(size);
    if(!new_buf)
        return 0;

    if(ptr) {
        int offset = (int)(ptr - kmalloc_pool);

        // Seek for the used block mapped to this offset
        int order = -1;
        int id = -1;
        int* s;
        find_used(offset, &order, &id, &s);

        // We never alloc'ed this block !
        if(order < 0 || id < 0)
            return 0;

        // Get the old buffer's size
        int old_size = blocks_size[order];

        // Copy data in the new buffer
        int min_size = old_size < size ? old_size : size;
        for(int i = 0; i < min_size; ++i)
            *((char*)new_buf + i) = *((char*)ptr + i);

        // Release the old buffer
        if(release(offset) < 0)
            return 0;
    }

    return new_buf;
}

void kfree(void* ptr) {
    if(!ptr)
        return;

    int offset = (int)(ptr - kmalloc_pool);
    release(offset);
}
