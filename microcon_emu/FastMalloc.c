/*
super simple yet super fast memory allocator for non mmu systems

requirements:
- needs to have closed to fixed time for allocation (RTOS requirement)
- needs less fragmentation
- simplicity (otherwise debugging will be a nightmare)
- must be thread safe AND performant at the same time.

basic ideas:
- separate classes per size (array for 512b, 4kb, 64kb, max kb...)
-- we can reduce fragmentation
- simple bitmaps for each containers.
-- we can count the leading zeroes of the bitmap using special opcodes
-- 32bit bitmap is not enough, so lets use dual layers (1 master map + 32 sub maps = 33 32bit variables = representing 1024 slots)
--- master map is only bit flipped when the corresponding sub map is completely filled. (checked by comparing the sub map with 0xFFFFFFFF)
--- on dealloc, we only need to bitflip the master to 0 without any other checks.

search ideas:
- simple for loop iteration to the end for systems with no special opcodes.
-- use clz or similar opcode for systems that support them.
--- good for first searches.
--- for once-freed blocks, we need a different approach.
---- 1. do the normal clz search (greedy approach). the location is the clz result.
---- 2. if clz returns 0 & master bit is still 0, invert the sub map and do clz again to find the freed block. the location is the clz result + 1.

armv7m: supports mvn(bitwise not for inversion) and clz(count leading zeroes)
rh850: supports sch0l(search 0 starting from msb) - this essentially emulates a full hardware for loop. (no extra logic needed for the once-freed blocks)
x86: supports bsf(bit scan forward)

two-layer allocator structure:
- every allocator has a master bitmap and 32 sub bitmaps, each representing 32 blocks. (so 1024 blocks per allocator)
-- 1 main allocator of 4kb blocks (not the smallest nor the biggest, but a middle ground)
-- sub allocators for each size classes (512b, 4kb, 64kb, max kb...) with their own bitmaps and memory pools. 
(but their pools are empty at the start, they are only filled when the main allocator gives them blocks)

coalescing ideas for sub allocator:
- lets say the sub allocator is for 64kb blocks.
- the allocator requests sequential blocks from the main allocator to be able to coalesce them into a 64kb block.

splitting ideas for sub allocator:
- lets say the sub allocator is for 512b blocks.
- the allocator must request one 4kb block from the main allocator
-- and split it into 8 512b blocks.
- on allocation
-- we check if the sub allocator has any blocks available
--- if not, we can request a block from the main allocator
(we keep track of the available size in the sub allocator <in one variable> to know when we need to request more blocks from the main allocator)

how to verify the given address on deallocation:
- we can range check with compiled symbols.
-- we can also check map location with it.

thread safety ideas:
- simply divide them per core.
-- with an added benefit of being able to set different allocator pool sizes per core and possibly align them to their local ram.

resource scheduling ideas (to select whether to use bigger blocks or smaller blocks coalesced):
- with the architecture above, we do best-fit allocation for selecting the block size class
-- and the class allocator will do the best fit search for the block within its class. 
(so we have a two level best fit allocation)
--- if we dont have any blocks in the class, we can try to get a block from the main allocator.
(with this design, theres no splitting nor coalescing)
--- we need to keep an extra variable to track the allocated size of the whole allocator. 
(so that the sub allocators can check if they have enough space to allocate a block from the main allocator)

freeing ideas for the main allocator:
- we design the sub allocators to simply actively(on free call) free the blocks back to the main allocator. (to keep the main allocator from being exhausted)
-- then we wont need to keep track of whether we already have the main allocator blocks allocated to the sub allocators.

*/

/*
prototype:



*/

/* log based allocator */

#include "FastMalloc.h"

uint32 logalloc_pool[MAX_POOL_SIZE];

/* for calloc */
void* logalloc_allocate_clear_memory(uint32 size)
{
    uint32* ptr = NULL;
    ptr = (uint32*)logalloc_allocate_memory(size);
    memset(ptr, 0, size);
    return ptr;
}

/* for malloc */
/* first 3(0;magic,1;previdx,2;nextidx) is header, 4+ is data.
* indexes point to header, not data. */
void* logalloc_allocate_memory(uint32 size)
{
    uint32 index = 0; /* must always point to header magic */
    uint32 blocksize = size / 4 + 3; /* +3 is mandatory header */
    uint32 curr_block_prev = 0;
    uint32 curr_block_next = 0;

    /* very first allocation */
    if (logalloc_pool[0] == 0){
        logalloc_pool[0] = 0xAAAAAAAA;
        logalloc_pool[1] = 0;
        logalloc_pool[2] = blocksize;   /* includes the header */
        logalloc_pool[blocksize] = 0xAAAAAAAA;
        logalloc_pool[blocksize + 1] = 0; /* prev is 0 since this is the first block */
        logalloc_pool[blocksize + 2] = 0; /* nothing after this block yet */

        return &logalloc_pool[3]; /* return data area */
    }

    /* we have at least one block here */
    while(1)
    {
        /* memory corruption detected */
        m_assert(logalloc_pool[index] == 0xAAAAAAAA, "memory corruption detected in logalloc pool");

        /* current block's header */
        curr_block_prev = logalloc_pool[index + 1];
        curr_block_next = logalloc_pool[index + 2];

        /* one iteration */
        /* check if we arrived at the end block (that we can append after) */
        if (curr_block_prev != 0x0 && curr_block_next == 0x0)
        {
            /* curr block */
            /* Current block at 'index' will become our new allocated block */
            logalloc_pool[index + 2] = index + blocksize; /* update next pointer to new end */
            
            /* next block */
            logalloc_pool[index + blocksize] = 0xAAAAAAAA;
            logalloc_pool[index + blocksize + 1] = index;
            logalloc_pool[index + blocksize + 2] = 0; /* next is 0 since this is the new end */

            return &logalloc_pool[index + 3]; /* return data area */
        }
        /* or we arrived in the middle of the list */
        else if (curr_block_prev != 0x0 && curr_block_next != 0x0)
        {
            /* make sure next block isnt corrupted */
            m_assert(logalloc_pool[curr_block_next] == 0xAAAAAAAA, "memory corruption detected in logalloc pool: next block header corrupted");
            /* gap detection: mfree removes block by simply doing prev_block->next = curr_block->next,
             * effectively skipping the freed block. 
             * next_block is untouched, so we can check the next_block->prev != prev_block to detect the gap. */
            uint32 alleged_prev = logalloc_pool[curr_block_next + 1];
            uint32 gap = curr_block_next - alleged_prev;
            if (gap >= blocksize)
            {
                /* we have enough space to insert a new block here */
                logalloc_pool[index + 2] = alleged_prev; /* update current block's next to point to new block */

                /* new block */
                logalloc_pool[alleged_prev] = 0xAAAAAAAA;
                logalloc_pool[alleged_prev + 1] = index; /* prev points to current block */
                logalloc_pool[alleged_prev + 2] = curr_block_next; /* next points to next block */

                /* next block */
                logalloc_pool[curr_block_next + 1] = alleged_prev; /* update next block's prev to point to new block */

                return &logalloc_pool[alleged_prev + 3]; /* return data area */
            }
            else
            {
                /* gap too small, keep looking */
            }
        }

        index = curr_block_next; /* move to next block */
    }
    
}