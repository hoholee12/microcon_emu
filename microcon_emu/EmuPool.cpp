#include "EmuPool.hpp"
#include <vector>
#include <ctime>

/* simple memory allocator for emulation structures
 * structure:
 * 1MB buffer
 * 
 * - header -
 * first 4byte: magic number for corruption detection (0xAAAAAAAA for allocated, 0x0 for freed)
 * second 4byte: previous block's starting address + 3 (starting address without the header)
 * third 4byte: next block's starting address
 * 
 * if a middle block is freed, previous block's next pointer is set to the next of the freed block
 * freed block's magic number is cleared to 0, next pointer is cleared to 0
 * IMPORTANT: freed block's prev pointer is PRESERVED (not cleared) for coalescing logic
 * ending address + 1 is the address to the first byte of previous block (needed to traverse backwards)
 * so for the single block, [0] is a magic number, [1] is previous block's starting address, [2] is next block's starting address, [3] is first byte of this block.
 *
 * for example: lets say we allocate 3 blocks of 0x2000 bytes each
 * - first block
 * [0] = 0xAAAAAAAA (magic number for corruption detection)
 * [1] = (previous block's starting address; which should be 0 if this is the first block)
 * [2] = 0x2003 (next block's starting address; 0x2000 + 3 for this block's header)
 * [3] = 0xXXXX (first byte of this block)
 * ...
 * [0x2002] = 0xXXXX (last byte of this block)
 * - second block
 * [0x2003] = 0xAAAAAAAA
 * [0x2004] = 0x3 (previous block's starting address) - 0 + 3
 * [0x2005] = 0x4006 (next block's starting address)
 * [0x2006] = 0xYYYY (first byte of this block)
 * ...
 * [0x4005] = 0xYYYY (last byte of this block)
 * - third block
 * [0x4006] = 0xAAAAAAAA
 * [0x4007] = 0x2009 (previous block's starting address)
 * [0x4008] = 0x6009 (next block's starting address; points to a place that doesn't exist, but is done anyway to indicate the end of the pool)
 * [0x4009] = 0xZZZZ (first byte of this block)
 * ...
 * [0x6008] = 0xZZZZ (last byte of this block)
 * - no blocks after this point
 * [0x6009] = 0xAAAAAAAA (assigned when inserting the third block)
 * [0x600A] = 0x400A (previous block's starting address; assigned when inserting the third block)
 * [0X600B] = 0x0
 * ...
 *  - that's it.
 * 
 * when allocating freed blocks, we can check for the free blocks by checking the [1] of the next block and [0] of the current block.
 * allocation always goes left -> right. it does not go reverse. we can use this fact to indicate an empty gap.
 * when we dealloc block B (between A and C):
 * block A -> update A's next pointer to point to block C (skip over freed B)
 * block B -> clear magic to 0, clear next to 0, but KEEP prev pointer
 * block C -> do NOT update C's prev pointer (still points to nonexistent block B)
 * 
 * Gap detection during allocation:
 * When traversing blocks, if we find that current_block.prev != expected_prev_addr,
 * it means there's a freed block between the previous block and current block.
 * The freed block's address is (current_block.prev - 3) since prev points to data, not header.
 * Gap size = current_block_addr - freed_block_addr
 * 
 * Block coalescing during deallocation:
 * Before marking a block as freed, check if adjacent blocks are already free:
 * 1. Check previous: if prev_block.next != our_header_addr, there's a gap before us
 *    - If prev_block.next == 0, the previous block is freed (walk backwards through chain)
 *    - Walk backwards using preserved prev pointers until we find an allocated block (magic == 0xAAAAAAAA)
 *    - This handles chains of multiple consecutive freed blocks
 * 2. Check next: if next_block.magic == 0, the next block is already freed
 *    - Scan forward through consecutive freed blocks to find the final next pointer
 * 3. When coalescing, update forward links to skip over all freed blocks in the chain
 * 4. Example: A -> [freed B] -> [freed C] -> [to-free D] -> E becomes A -> E
 *    - When freeing D, walk backwards through C and B to find A
 *    - Update A's next pointer to point to E, skipping over B, C, and D
 * 5. This creates larger contiguous free spaces for bigger allocations
 * 6. The preserved prev pointers in freed blocks enable this backward traversal
 */

uint32 logalloc_pool[MAX_POOL_SIZE];
uint32 last_pos = 0;
uint32 first_alloc = 0;
uint32 logalloc_pool_cap = 0;
uint32 last_alloc_pos = MAX_POOL_SIZE - 4;

void logalloc_init() {
    memset(logalloc_pool, 0, sizeof(logalloc_pool));
    last_pos = 0;
    first_alloc = 0;
    logalloc_pool_cap = 0;
    
    /* allocate the first block for wraparound sentinel */
    logalloc_allocate_memory(4);
}

/* for calloc */
void* logalloc_allocate_clear_memory(uint32 size)
{
    uint32* ptr = NULL;
    ptr = (uint32*)logalloc_allocate_memory(size);
    memset(ptr, 0, size);
    return ptr;
}

/* for free */
void logalloc_free_memory(void* ptr)
{
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - 3; /* get header index from data pointer */
    m_assert(logalloc_pool[baseindex] == 0xAAAAAAAA, 
        "memory corruption detected in logalloc pool, or you are passing an invalid pointer");
    
    m_assert(((baseindex != 0) && (baseindex != last_alloc_pos)), 
        "you cannot free the first or last block of the logalloc pool "
        "since it is used as a sentinel for wraparound");

    /* coalesce */
    uint32 prevblock_startaddr = logalloc_pool[baseindex + 1];
    uint32 nextblock_startaddr = logalloc_pool[baseindex + 2];
    /* update previous block's next to skip current block */
    logalloc_pool[prevblock_startaddr + 2] = nextblock_startaddr;
    /* destroy */
    logalloc_pool[baseindex] = 0; /* clear magic to mark as free */
    logalloc_pool[baseindex + 1] = 0; /* clear prev pointer */
    logalloc_pool[baseindex + 2] = 0; /* clear next pointer */
    last_pos = prevblock_startaddr; /* rewind pos */
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

    /* very first allocation - non-freeable - for wraparound
     * you could allocate a huge amount of memory here,
     * or be a smart user and just allocate one block for the wraparound sentinel. */
    if (first_alloc == 0){
        logalloc_pool[0] = 0xAAAAAAAA;
        logalloc_pool[1] = last_alloc_pos;
        logalloc_pool[2] = last_alloc_pos;
        logalloc_pool[last_alloc_pos] = 0xAAAAAAAA; /* end block for wraparound */
        logalloc_pool[last_alloc_pos + 1] = blocksize; /* prev points to the first block */
        logalloc_pool[last_alloc_pos + 2] = 0; /* wraparound to index 0 */
        /* MAX_POOL_SIZE - 1 is empty and is only there for 16byte align */

        first_alloc = 1;    /* this will never reset until program termination */
        last_pos = 0;
        logalloc_pool_cap = blocksize;
        return &logalloc_pool[3]; /* return data area */
    }

    /* sanity check */
    m_assert(logalloc_pool_cap + blocksize < MAX_POOL_SIZE, "logalloc pool out of memory");

    /* start searching from last position for better performance */
    index = last_pos;

    /* we have at least one block here */
    while(1)
    {
        /* memory corruption detected */
        m_assert(logalloc_pool[index] == 0xAAAAAAAA, "memory corruption detected in logalloc pool");

        /* current block's header */
        curr_block_prev = logalloc_pool[index + 1];
        curr_block_next = logalloc_pool[index + 2];

        /* one iteration */
        /* we searching in the middle of the list */
        if (curr_block_next != 0x0)
        {
            /* make sure next block isnt corrupted */
            m_assert(logalloc_pool[curr_block_next] == 0xAAAAAAAA, 
                "memory corruption detected in logalloc pool: next block header corrupted");
            /* gap detection: mfree removes block by simply doing prev_block->next = curr_block->next,
             * effectively skipping the freed block. 
             * next_block is untouched, so we can check the next_block->prev != prev_block to detect the gap. */
            uint32 alleged_prev = logalloc_pool[curr_block_next + 1];
            uint32 gap = curr_block_next - alleged_prev;
            if (gap >= blocksize)
            {
                /* we have enough space to insert a new block here */
                logalloc_pool[index + 2] = alleged_prev; /* update current block's next to point to new block */
                last_pos = alleged_prev;
                logalloc_pool_cap += blocksize;
                /* new block */
                logalloc_pool[alleged_prev] = 0xAAAAAAAA;
                logalloc_pool[alleged_prev + 1] = index; /* prev points to current block */
                /* in case we allocate smaller than gap, we still need gap logic for future alloc here */
                logalloc_pool[alleged_prev + 2] = curr_block_next; /* next points to next block */
                if (gap == blocksize)
                {
                    /* perfect fit, we can just update the next block's prev to point to new block */
                    logalloc_pool[curr_block_next + 1] = alleged_prev;
                }
                else
                {
                    /* we can fit more, we need to update the next block's prev to point to new block's next
                     * - we plant gap logic for future allocs here */
                    logalloc_pool[curr_block_next + 1] = alleged_prev + blocksize;
                }

                return &logalloc_pool[alleged_prev + 3]; /* return data area */
            }
            else
            {
                /* gap too small, keep looking */
            }
        }
        else
        {
            /* check if we arrived at the end block (wraparound) */
            if (curr_block_prev != 0x0)
            {
                curr_block_next = logalloc_pool[curr_block_next + 2];
            }
            else if (curr_block_prev == 0x0)
            {
                /* this should never happen, means we have a corrupted block with invalid prev/next pointers */
                m_assert(0, "memory corruption detected in logalloc pool: invalid prev/next pointers");
            }
        }

        index = curr_block_next; /* move to next block */
        /* looped the whole pool and couldnt find a single spot to spare. */
        m_assert(index != last_pos, "searched the logalloc pool far and wide "
            "but could not find a consecutive block to spare");
    }
    
}
