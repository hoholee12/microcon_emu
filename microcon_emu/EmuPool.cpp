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

uint32 logalloc_pool[MAX_POOL_SIZE / sizeof(uint32)];
uint32 last_pos_perf_penalty = 0;    /* performance metric for last_pos misses */
INDEX_TYPE last_pos = 0;    /* last position for better performance */
INDEX_TYPE last_alloc_pos = (MAX_POOL_SIZE - sizeof(logalloc_block_header)) / sizeof(uint32);
INDEX_TYPE logalloc_pool_cap = 0;


/* start of performance stuff */
uint32 perf_blockcnt = 4;
uint32 perf_blocksizes[] = {16, 256, 4096, 65536};
uint32 perf_blockpercentage[] = {90, 5, 3, 2};
void* perf_blockptr[4]; /* only for temporary blocks for freeing */

void logalloc_perfinit()
{
    /* allocate artificial size class fences */
    /* if the percentage does not add up(overflow), it will assert */
    /* watch out for underflow ! */
    uint32 medium_size = (MAX_POOL_SIZE / 100 * perf_blockpercentage[0]) - sizeof(logalloc_block_header);
    perf_blockptr[0] = logalloc_allocate_memory(medium_size);
    for (uint32 i = 1; i < perf_blockcnt - 1; i++)
    {
        medium_size = (MAX_POOL_SIZE / 100 * perf_blockpercentage[i]) - sizeof(logalloc_block_header);
        logalloc_allocate_memory(0);
        perf_blockptr[i] = logalloc_allocate_memory(medium_size);
    }
    medium_size = (MAX_POOL_SIZE / 100 * perf_blockpercentage[perf_blockcnt - 1]) - (sizeof(logalloc_block_header) * 2);
    logalloc_allocate_memory(0);
    perf_blockptr[perf_blockcnt - 1] = logalloc_allocate_memory(medium_size);

    /* free the temporary blocks */
    for (uint32 i = 0; i < perf_blockcnt; i++)
    {
        logalloc_free_memory(perf_blockptr[i]);
    }

    last_pos_perf_penalty = 0;
    last_pos = 0;
}



/* end of performance stuff */

void logalloc_init()
{
    logalloc_block_header* curr_header;
    INDEX_TYPE blocksize = sizeof(logalloc_block_header) / sizeof(uint32); /* the beginning and the end */

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
    logalloc_pool_cap = blocksize * 2;

    logalloc_perfinit();
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
    INDEX_TYPE nextindex = 0;
    m_assert(curr_header->magic == MAGIC_NUMBER, "memory corruption, or you are passing an invalid pointer");
    
    m_assert(((baseindex != 0) && (baseindex != last_alloc_pos)), 
        "you cannot free the first or last block of the logalloc pool "
        "since it is used as a sentinel for wraparound");

    INDEX_TYPE prevblock_startidx = curr_header->prev;
    INDEX_TYPE nextblock_startidx = curr_header->next;

    logalloc_block_header* prevblock_header = CONV_IDX_TO_ADDR(prevblock_startidx);
    logalloc_block_header* nextblock_header = CONV_IDX_TO_ADDR(nextblock_startidx);

    /* coalesce if next block is a freed block */
    if (CONV_IDX_TO_ADDR(nextblock_header->prev)->magic == MAGIC_NUMBER_FREE)
    {
        nextindex = nextblock_header->prev;
    }
    else
    {
        nextindex = nextblock_startidx;
    }

    /* coalesce if previous block is a freed block */
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
    uint32 size_to_subtract = (nextindex - baseindex);
    logalloc_pool_cap -= size_to_subtract; /* update capacity */
    /* TODO: capacity calc becomes weird when wraparound occurs */
}

/* for malloc */
/* first 3(0;magic,1;previdx,2;nextidx) is header, 4+ is data.
* indexes point to header, not data. */
void* logalloc_allocate_memory(uint32 bytecount)
{
    INDEX_TYPE curr_index = 0; /* must always point to header magic */
    INDEX_TYPE blocksize = ((INDEX_TYPE)bytecount + sizeof(logalloc_block_header)) / sizeof(uint32); /* blocksize must be in byte units */
    INDEX_TYPE curr_index_prev, curr_index_next;
    logalloc_block_header *curr_header, *curr_header_prev, *curr_header_next, *gap_header;

    /* sanity check */
    m_assert(logalloc_pool_cap + blocksize < MAX_POOL_SIZE, "logalloc pool out of memory");

    /* start searching from last position for better performance */
    curr_index = last_pos;

    /* validate last_pos first */
    if (CONV_IDX_TO_ADDR(last_pos)->magic != MAGIC_NUMBER)
    {
        /* if its not a valid address, start from zero and increment penalty counter */
        last_pos = 0;
        last_pos_perf_penalty++;
    }

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
                    uint32 post_gap_index = gap_index + blocksize;
                    curr_header_next->prev = post_gap_index;

                    /* if we were to deallocate, make it a lone island, 
                     * and then try reallocating the left side with a smaller block; */
                    /* we need to plant free magic here */
                    logalloc_block_header* post_gap_header = CONV_IDX_TO_ADDR(post_gap_index);
                    post_gap_header->magic = MAGIC_NUMBER_FREE;
                    post_gap_header->prev = gap_index;
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
