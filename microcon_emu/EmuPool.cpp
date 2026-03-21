#include "EmuPool.hpp"
#include <vector>
#include <ctime>

/* simple memory allocator for emulation structures
 * structure:
 * 1MB buffer
 * 
 * - header -
 * first 4byte: magic number for corruption detection (0xAAAAAAAA for allocated, 0xBBBBBBBB for freed)
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
 * block B -> clear magic to 0xBBBBBBBB and KEEP prev pointer
 * block C -> should point to the now removed block B
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
 *    - Walk backwards using preserved prev pointers until we find an allocated block 
 *      (magic == 0xAAAAAAAA)
 *    - This handles chains of multiple consecutive freed blocks
 * 2. Check next: if next_block.magic == 0xBBBBBBBB, the next block is already freed
 *    - Scan forward through consecutive freed blocks to find the final next pointer
 * 3. When coalescing, update forward links to skip over all freed blocks in the chain
 * 4. Example: A -> [freed B] -> [freed C] -> [to-free D] -> E becomes A -> E
 *    - When freeing D, walk backwards through C and B to find A
 *    - Update A's next pointer to point to E, skipping over B, C, and D
 * 5. This creates larger contiguous free spaces for bigger allocations
 * 6. The preserved prev pointers in freed blocks enable this backward traversal
 * 
 * We put wraparound sentinels at each ends of the pool array (two empty pre-allocated blocks)
 * so that the search can start anywhere on the pool, and wraparound to keep searching until 
 * one cycle is complete without the pool end limit.
 */

uint32 logalloc_pool[MAX_POOL_SIZE];
uint32 logalloc_pool_cap = 0;
INDEX_TYPE last_pos = 0;
INDEX_TYPE last_alloc_pos = MAX_POOL_SIZE - 4;

void logalloc_init() {
    logalloc_block_header* curr_header;
    INDEX_TYPE blocksize = sizeof(logalloc_block_header);

    memset(logalloc_pool, 0, sizeof(logalloc_pool));
    last_pos = 0;
    logalloc_pool_cap = 0;
    
    /* allocate the first block for wraparound sentinel */
    curr_header = CONV_IDX_TO_ADDR(0);
    curr_header->magic = MAGIC_NUMBER;
    curr_header->prev = last_alloc_pos;
    curr_header->next = last_alloc_pos;

    curr_header = CONV_IDX_TO_ADDR(last_alloc_pos);
    curr_header->magic = MAGIC_NUMBER; /* end block for wraparound */
    curr_header->prev = blocksize; /* prev points to the first block */
    curr_header->next = 0; /* wraparound to index 0 */

    last_pos = 0;
    logalloc_pool_cap = blocksize;
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
    INDEX_TYPE baseindex = ((uint32*)ptr - logalloc_pool) - (sizeof(logalloc_block_header) / sizeof(uint32)); /* get header index from data pointer */
    logalloc_block_header* curr_header = CONV_IDX_TO_ADDR(baseindex);
    m_assert(curr_header->magic == MAGIC_NUMBER, "you are passing an invalid pointer");
    
    m_assert(((baseindex != 0) && (baseindex != last_alloc_pos)), 
        "you cannot free the first or last block of the logalloc pool "
        "since it is used as a sentinel for wraparound");

    INDEX_TYPE prevblock_startidx = curr_header->prev;
    INDEX_TYPE nextblock_startidx = curr_header->next;

    /* coalesce if previous block is a freed block */
    logalloc_block_header* prevblock_header = CONV_IDX_TO_ADDR(prevblock_startidx);
    logalloc_block_header* nextblock_header = CONV_IDX_TO_ADDR(nextblock_startidx);

    if (prevblock_header->magic == MAGIC_NUMBER_FREE)
    {
        CONV_IDX_TO_ADDR(prevblock_header->prev)->next = nextblock_startidx;
        nextblock_header->prev = prevblock_startidx;
        last_pos = prevblock_header->prev; /* double rewind pos */
    }
    else
    {
        prevblock_header->next = nextblock_startidx;
        nextblock_header->prev = baseindex;
        last_pos = prevblock_startidx; /* rewind pos */
    }
    
    /* destroy */
    curr_header->magic = MAGIC_NUMBER_FREE; /* mark as freed */
    logalloc_pool_cap -= (nextblock_startidx - baseindex); /* update capacity */
}

/* for malloc */
/* first 3(0;magic,1;previdx,2;nextidx) is header, 4+ is data.
* indexes point to header, not data. */
void* logalloc_allocate_memory(uint32 size)
{
    INDEX_TYPE curr_index = 0; /* must always point to header magic */
    INDEX_TYPE blocksize = (INDEX_TYPE)size / 4 + sizeof(logalloc_block_header);
    INDEX_TYPE curr_index_prev, curr_index_next;
    logalloc_block_header *curr_header, *curr_header_prev, *curr_header_next, *gap_header;

    /* sanity check */
    m_assert(logalloc_pool_cap + blocksize < MAX_POOL_SIZE, "logalloc pool out of memory");

    /* start searching from last position for better performance */
    curr_index = last_pos;

    /* we have at least one block here */
    while(1)
    {
        curr_header = CONV_IDX_TO_ADDR(curr_index);
        /* memory corruption detected */
        m_assert(curr_header->magic == MAGIC_NUMBER, "memory corruption detected in logalloc pool");

        curr_index_prev = curr_header->prev;
        curr_index_next = curr_header->next;

        /* one iteration */
        /* we searching in the middle of the list */
        if (curr_index_next != 0x0)
        {
            /* current block's header */
            curr_header_prev = CONV_IDX_TO_ADDR(curr_index_prev);
            curr_header_next = CONV_IDX_TO_ADDR(curr_index_next);
            /* make sure next block isnt corrupted */
            m_assert(curr_header_next->magic == MAGIC_NUMBER, 
                "memory corruption detected in logalloc pool: next block header corrupted");
            /* gap detection: mfree removes block by simply doing prev_block->next = curr_block->next,
             * effectively skipping the freed block. 
             * next_block is untouched, so we can check the next_block->prev != prev_block to detect the gap. */
            INDEX_TYPE gap_index = curr_header_next->prev;
            INDEX_TYPE gapsize = curr_index_next - gap_index;
            if (gapsize >= blocksize)
            {
                /* we have enough space to insert a new block here */
                curr_header->next = gap_index; /* update current block's next to point to new block */
                last_pos = gap_index;
                logalloc_pool_cap += blocksize;
                /* new block */
                gap_header = CONV_IDX_TO_ADDR(gap_index);
                gap_header->magic = MAGIC_NUMBER;
                gap_header->prev = curr_index; /* prev points to current block */
                /* in case we allocate smaller than gap, we still need gap logic for future alloc here */
                gap_header->next = curr_index_next; /* next points to next block */
                if (gapsize == blocksize)
                {
                    /* perfect fit, we can just update the next block's prev to point to new block */
                    curr_header_next->prev = gap_index;
                }
                else
                {
                    /* we can fit more, we need to update the next block's prev to point to new block's next
                     * - we plant gap logic for future allocs here */
                    curr_header_next->prev = gap_index + blocksize;
                }

                return CONV_ADDR_TO_BODY(gap_header); /* return data area */
            }
            else
            {
                /* gap too small, keep looking */
            }
        }
        else
        {
            /* check if we arrived at the end block (wraparound) */
            if (curr_index_prev != 0x0)
            {
                curr_index_next = CONV_IDX_TO_ADDR(curr_index_next)->next;
            }
            else if (curr_index_prev == 0x0)
            {
                /* this should never happen, means we have a corrupted block with invalid prev/next pointers */
                m_assert(0, "memory corruption detected in logalloc pool: invalid prev/next pointers");
            }
        }

        curr_index = curr_index_next; /* move to next block */
        /* looped the whole pool and couldnt find a single spot to spare. */
        m_assert(curr_index != last_pos, "searched the logalloc pool far and wide "
            "but could not find a consecutive block to spare");
    }
    
}

void* logalloc_realloc_memory(void* ptr, uint32 size)
{
    void* temp = logalloc_allocate_memory(size);
    memcpy(temp, ptr, size);

    logalloc_free_memory(ptr);
    return temp;
}
