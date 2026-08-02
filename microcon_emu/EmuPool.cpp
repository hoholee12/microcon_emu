#include "EmuPool.hpp"
#include <vector>
#include <ctime>
#include <stdio.h>

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
uint32 last_pos = 0;    /* last position for better performance */
uint32 last_alloc_pos = (MAX_POOL_SIZE - sizeof(logalloc_block_header)) / sizeof(uint32);
uint32 logalloc_pool_cap = 0;

#ifdef RELATIVE_INDEXING
void logalloc_init()
{
    uint32 prev_pos = 0;
    uint32 prev_gap_pos = 0;
    uint32 curr_pos = 0;
    uint32 curr_gap_pos = 0;

    uint32 blocksize = sizeof(logalloc_block_header) / sizeof(uint32); /* the beginning and the end */
    prev_gap_pos = blocksize; /* the first gap block is after the first sentinel block */

    memset(logalloc_pool, 0, sizeof(logalloc_pool));
    last_pos = 0;
    logalloc_pool_cap = 0;

    if (last_alloc_pos <= UINT24_MAX)
    {
        /* allocate the first block for wraparound sentinel */
        RELADR_HEAD_UPDATE(0, blocksize, last_alloc_pos); /* first: prevoffset underflows to last_alloc, nextoffset skips gap to last_alloc */
        /* create a free block in the middle to represent the gap for gap detection */
        RELADR_HEAD_UPDATE_FREE(blocksize, blocksize); /* gap: prevoffset to first sentinel (not in forward chain) */
        RELADR_HEAD_UPDATE(last_alloc_pos, last_alloc_pos - blocksize, blocksize); /* last: prevoffset to gap, nextoffset wraps to 0 */
    }
    else
    {
        RELADR_HEAD_UPDATE(0, blocksize, UINT24_MAX); /* first block for wraparound */
        while(prev_pos + UINT24_MAX < last_alloc_pos)
        {
            curr_pos = prev_pos + UINT24_MAX;
            curr_gap_pos = prev_gap_pos + UINT24_MAX;

            RELADR_HEAD_UPDATE(curr_pos, UINT24_MAX, UINT24_MAX);
            RELADR_HEAD_UPDATE_FREE(curr_gap_pos, blocksize);

            prev_pos = curr_pos;
            prev_gap_pos = curr_gap_pos;
            logalloc_pool_cap += blocksize;
        }

        /* last block */
        RELADR_HEAD_UPDATE(curr_pos, UINT24_MAX, last_alloc_pos - curr_pos); /* one last middle sentinel and a gap */
        RELADR_HEAD_UPDATE_FREE(curr_gap_pos, blocksize);

        RELADR_HEAD_UPDATE(last_alloc_pos, last_alloc_pos - curr_pos, blocksize); /* end block for wraparound */

    }

    last_pos = 0;
    logalloc_pool_cap += blocksize * 3;
    last_pos_perf_penalty = 0;
}
#else
void logalloc_init()
{
    logalloc_block_header* curr_header;
    uint32 blocksize = sizeof(logalloc_block_header) / sizeof(uint32); /* the beginning and the end */

    memset(logalloc_pool, 0, sizeof(logalloc_pool));
    last_pos = 0;
    logalloc_pool_cap = 0;
    
    /* allocate the first block for wraparound sentinel */
    curr_header = CONV_IDX_TO_ADDR(0);
    curr_header->magic = MAGIC_NUMBER;
    curr_header->prev = last_alloc_pos;
    curr_header->next = last_alloc_pos;

    /* create a free block in the middle to represent the gap for gap detection */
    curr_header = CONV_IDX_TO_ADDR(blocksize);
    curr_header->magic = MAGIC_NUMBER_FREE;
    curr_header->prev = 0;
    curr_header->next = 0; /* free blocks dont have a defined next index */

    curr_header = CONV_IDX_TO_ADDR(last_alloc_pos);
    curr_header->magic = MAGIC_NUMBER; /* end block for wraparound */
    curr_header->prev = blocksize; /* prev points to the gap block */
    curr_header->next = 0; /* wraparound to index 0 */

    last_pos = 0;
    logalloc_pool_cap = blocksize * 3;

    last_pos_perf_penalty = 0;
}
#endif

/* for calloc */
void* logalloc_allocate_clear_memory(uint32 size)
{
    uint32* ptr = NULL;
    ptr = (uint32*)logalloc_allocate_memory(size);
    memset(ptr, 0, size);
    return ptr;
}

/* for free */
#ifdef RELATIVE_INDEXING
void logalloc_free_memory(void* ptr)
{
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - (sizeof(logalloc_block_header) / sizeof(uint32)); /* get header index from data pointer */
    uint32 real_nextindex = 0;
    uint32 magic_num = RELADR_MAGIC_NUMBER(baseindex);
    m_assert(magic_num == MAGIC_NUMBER, "memory corruption, or you are passing an invalid pointer");
    
    m_assert(((baseindex != 0) && (baseindex != last_alloc_pos)), 
        "you cannot free the first or last block of the logalloc pool "
        "since it is used as a sentinel for wraparound");

    uint32 prevblock_startidx = RELADR_PREV_IDX(baseindex);
    uint32 nextblock_startidx = RELADR_NEXT_IDX(baseindex);
    uint32 nextblock_startoffset = RELADR_NEXT_OFFSET(baseindex);
    uint32 prevblock_startoffset = RELADR_PREV_OFFSET(baseindex);
    uint32 prevblock_prev_startoffset = RELADR_PREV_OFFSET(prevblock_startidx);

    uint32 nextblock_prev = RELADR_PREV_IDX(nextblock_startidx);
    uint32 prevblock_magic = RELADR_MAGIC_NUMBER(prevblock_startidx);
    uint32 prevblock_prev = RELADR_PREV_IDX(prevblock_startidx);
    uint32 prevblock_next = RELADR_NEXT_IDX(prevblock_startidx);

    /* nextindex for calculating and subtracting current block size
     * there can be a gap between current and next allocated */
    if (RELADR_MAGIC_NUMBER(nextblock_prev) == MAGIC_NUMBER_FREE)
    {
        real_nextindex = nextblock_prev;
    }
    /* ...or no gap */
    else
    {
        real_nextindex = nextblock_startidx;
    }

    /* 1 step coalesce when previous block is also a freed block
     * (necessary to maintain links for all allocated blocks) */
    if (prevblock_magic == MAGIC_NUMBER_FREE)
    {
        RELADR_HEAD_UPDATE(prevblock_prev, RELADR_PREV_OFFSET(prevblock_prev), nextblock_startoffset + prevblock_startoffset + prevblock_prev_startoffset);
        RELADR_HEAD_UPDATE(nextblock_startidx, nextblock_startoffset + prevblock_startoffset, RELADR_NEXT_OFFSET(nextblock_startidx)); /* new gap */
        last_pos = prevblock_prev; /* double rewind pos */
    }
    else
    {
        RELADR_HEAD_UPDATE(prevblock_startidx, RELADR_PREV_OFFSET(prevblock_startidx), nextblock_startoffset + prevblock_startoffset);
        RELADR_HEAD_UPDATE(nextblock_startidx, nextblock_startoffset, RELADR_NEXT_OFFSET(nextblock_startidx)); /* new gap */
        last_pos = prevblock_startidx; /* rewind pos */
    }

    /* destroy */
    RELADR_HEAD_UPDATE_FREE(baseindex, RELADR_PREV_OFFSET(baseindex)); /* mark as freed */
    logalloc_pool_cap -= (real_nextindex - baseindex); /* update capacity */
}
#else
void logalloc_free_memory(void* ptr)
{
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - (sizeof(logalloc_block_header) / sizeof(uint32)); /* get header index from data pointer */
    logalloc_block_header* curr_header = CONV_IDX_TO_ADDR(baseindex);
    uint32 real_nextindex = 0;
    m_assert(curr_header->magic == MAGIC_NUMBER, "memory corruption, or you are passing an invalid pointer");
    
    m_assert(((baseindex != 0) && (baseindex != last_alloc_pos)), 
        "you cannot free the first or last block of the logalloc pool "
        "since it is used as a sentinel for wraparound");

    uint32 prevblock_startidx = curr_header->prev;
    uint32 nextblock_startidx = curr_header->next;

    logalloc_block_header* prevblock_header = CONV_IDX_TO_ADDR(prevblock_startidx);
    logalloc_block_header* nextblock_header = CONV_IDX_TO_ADDR(nextblock_startidx);

    /* nextindex for calculating and subtracting current block size
     * there can be a gap between current and next allocated */
    if (CONV_IDX_TO_ADDR(nextblock_header->prev)->magic == MAGIC_NUMBER_FREE)
    {
        real_nextindex = nextblock_header->prev;
    }
    /* ...or no gap */
    else
    {
        real_nextindex = nextblock_startidx;
    }

    /* 1 step coalesce when previous block is also a freed block
     * (necessary to maintain links for all allocated blocks) */
    if (prevblock_header->magic == MAGIC_NUMBER_FREE)
    {
        CONV_IDX_TO_ADDR(prevblock_header->prev)->next = nextblock_startidx;
        nextblock_header->prev = prevblock_startidx; /* new gap */
        last_pos = prevblock_header->prev; /* double rewind pos */
    }
    /* prev is an allocated block; we can simply link prev - next */
    else
    {
        prevblock_header->next = nextblock_startidx;
        nextblock_header->prev = baseindex; /* new gap */
        last_pos = prevblock_startidx; /* rewind pos */
    }

    /* destroy */
    curr_header->magic = MAGIC_NUMBER_FREE; /* mark as freed */
    curr_header->next = 0; /* free blocks dont have a defined next index */
    logalloc_pool_cap -= (real_nextindex - baseindex); /* update capacity */
}
#endif

/* for malloc */
/* first 3(0;magic,1;previdx,2;nextidx) is header, 4+ is data.
* indexes point to header, not data. */

/* TODO: allocation with alignment
 * need a poke-hole type of logic in the allocation
 *
 * poke-hole:
 * 1. iter search
 * 2. check current index alignment
 * 
 * 2-1. if current index already aligned
 * 2-1-1. gap is small, go back to 1
 * 2-1-2. gap is big enough, allocate and return
 * 
 * 2-2. if current index is unaligned
 * 2-2-1. align current index to next aligned index
 * 2-2-2. check if gap is big enough to allocate
 * 2-2-3. if gap is big enough, allocate(poke-hole logic) and return
 * 2-2-4. if gap is too small, go back to 1
 * 
 * note: aligned index should consider the header size, so we need to add the header size to the current index before aligning it to the next aligned index.
 * (and then subtract the header size again for header allocation)
 * 
 * *assuming header is 2 words (relative indexing)
 * aligned_index = (current_index + (align_bytes / sizeof(uint32) - 1)) & ~(align_bytes / sizeof(uint32) - 1)
 * ex) current_index = 0x1234 (index after header = 0x1236), align_bytes = 16
 * aligned_index = (0x1236 + (16 / 4 - 1)) & ~(16 / 4 - 1) = (0x1236 + 3) & ~3 = 0x1239 & ~3 = 0x1238 (snap to next aligned index)
 * ex) current_index = 0x1232 (index after header = 0x1234), align_bytes = 16
 * aligned_index = (0x1234 + 3) & ~3 = 0x1237 & ~3 = 0x1234 (already aligned)
 * 
 */
#ifdef RELATIVE_INDEXING
void* logalloc_allocate_memory(uint32 bytecount, uint32 align_bytes)
{
    uint32 curr_index = 0; /* must always point to header magic */
    uint32 blocksize = ((uint32)bytecount + sizeof(logalloc_block_header)) / sizeof(uint32); /* blocksize must be in word units */
    uint32 curr_index_prev, curr_index_next;

    /* sanity check */
    m_assert(logalloc_pool_cap + blocksize < (MAX_POOL_SIZE / sizeof(uint32)), "logalloc pool out of memory");

    /* start searching from last position for better performance */
    curr_index = last_pos;

    /* validate last_pos first */
    if (RELADR_MAGIC_NUMBER(last_pos) != MAGIC_NUMBER)
    {
        /* if its not a valid address, start from zero and increment penalty counter */
        last_pos = 0;
        last_pos_perf_penalty++;
    }

    /* we have at least one block here */
    while(1)
    {
        /* memory corruption detected */
        m_assert(RELADR_MAGIC_NUMBER(curr_index) == MAGIC_NUMBER, "memory corruption detected in logalloc pool");

        curr_index_prev = RELADR_PREV_IDX(curr_index);
        curr_index_next = RELADR_NEXT_IDX(curr_index);

        /* one iteration */
        /* we searching in the middle of the list */
        if (curr_index_next != 0x0)
        {
            /* make sure next block isnt corrupted */
            m_assert(RELADR_MAGIC_NUMBER(curr_index_next) == MAGIC_NUMBER, 
                "memory corruption detected in logalloc pool: next block header corrupted");
            /* gap detection: mfree removes block by simply doing prev_block->next = curr_block->next,
             * effectively skipping the freed block. 
             * next_block is untouched, so we can check the next_block->prev != prev_block to detect the gap. */
            uint32 gap_index = RELADR_PREV_IDX(curr_index_next);
            uint32 gapsize = curr_index_next - gap_index;
            uint32 currsize = gap_index - curr_index;
            
            /* calculate the actual allocation index (aligned or not) */
            uint32 alloc_index = gap_index;
            if (align_bytes > 0)
            {
                /* calculate aligned position within this gap */
                uint32 header_words = sizeof(logalloc_block_header) / sizeof(uint32);
                uint32 align_words = align_bytes / sizeof(uint32);
                uint32 gap_data_index = gap_index + header_words;
                uint32 aligned_data_index = (gap_data_index + (align_words - 1)) & ~(align_words - 1);
                alloc_index = aligned_data_index - header_words;
            }
            
            /* check if allocation (aligned or not) fits in this gap */
            if (alloc_index >= gap_index && alloc_index + blocksize <= curr_index_next)
            {
                /* we have enough space to insert a new block here */
                uint32 pre_gap_size = alloc_index - gap_index;
                uint32 post_gap_size = curr_index_next - (alloc_index + blocksize);
                
                /* update current block to point to allocation or pre-gap */
                if (pre_gap_size > 0)
                {
                    /* there's space before the aligned block, create a free block */
                    RELADR_HEAD_UPDATE(curr_index, RELADR_PREV_OFFSET(curr_index), currsize);
                    RELADR_HEAD_UPDATE_FREE(gap_index, currsize); /* pre-gap free block */
                }
                else
                {
                    /* no pre-gap, current block points directly to allocation */
                    RELADR_HEAD_UPDATE(curr_index, RELADR_PREV_OFFSET(curr_index), alloc_index - curr_index);
                }
                
                last_pos = alloc_index;
                logalloc_pool_cap += blocksize;
                
                /* create the new allocated block */
                uint32 alloc_prevoffset = (pre_gap_size > 0) ? pre_gap_size : (alloc_index - curr_index);
                uint32 alloc_nextoffset = (post_gap_size > 0) ? blocksize : (curr_index_next - alloc_index);
                RELADR_HEAD_UPDATE(alloc_index, alloc_prevoffset, alloc_nextoffset);
                
                /* handle post-gap if exists */
                if (post_gap_size > 0)
                {
                    /* create a free block after the allocation */
                    uint32 post_gap_index = alloc_index + blocksize;
                    RELADR_HEAD_UPDATE_FREE(post_gap_index, blocksize);
                    RELADR_HEAD_UPDATE(curr_index_next, post_gap_size, RELADR_NEXT_OFFSET(curr_index_next));
                }
                else
                {
                    /* perfect fit, update next block's prev */
                    RELADR_HEAD_UPDATE(curr_index_next, alloc_nextoffset, RELADR_NEXT_OFFSET(curr_index_next));
                }

                return CONV_ADDR_TO_BODY(CONV_IDX_TO_ADDR(alloc_index)); /* return data area */
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
                curr_index_next = RELADR_NEXT_IDX(curr_index_next);
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
#else
void* logalloc_allocate_memory(uint32 bytecount, uint32 align_bytes)
{
    uint32 curr_index = 0; /* must always point to header magic */
    uint32 blocksize = ((uint32)bytecount + sizeof(logalloc_block_header)) / sizeof(uint32); /* blocksize must be in word units */
    uint32 curr_index_prev, curr_index_next;
    logalloc_block_header *curr_header, *curr_header_prev, *curr_header_next, *gap_header;

    /* sanity check */
    m_assert(logalloc_pool_cap + blocksize < (MAX_POOL_SIZE / sizeof(uint32)), "logalloc pool out of memory");

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
            uint32 gap_index = curr_header_next->prev;
            uint32 gapsize = curr_index_next - gap_index;
            
            /* calculate the actual allocation index (aligned or not) */
            uint32 alloc_index = gap_index;
            if (align_bytes > 0)
            {
                /* calculate aligned position within this gap */
                uint32 header_words = sizeof(logalloc_block_header) / sizeof(uint32);
                uint32 align_words = align_bytes / sizeof(uint32);
                uint32 gap_data_index = gap_index + header_words;
                uint32 aligned_data_index = (gap_data_index + (align_words - 1)) & ~(align_words - 1);
                alloc_index = aligned_data_index - header_words;
            }
            
            /* check if allocation (aligned or not) fits in this gap */
            if (alloc_index >= gap_index && alloc_index + blocksize <= curr_index_next)
            {
                /* we have enough space to insert a new block here */
                uint32 pre_gap_size = alloc_index - gap_index;
                uint32 post_gap_size = curr_index_next - (alloc_index + blocksize);
                
                /* update current block to point to allocation or pre-gap */
                if (pre_gap_size > 0)
                {
                    /* there's space before the aligned block, create a free block */
                    curr_header->next = gap_index;
                    logalloc_block_header* pre_gap_header = CONV_IDX_TO_ADDR(gap_index);
                    pre_gap_header->magic = MAGIC_NUMBER_FREE;
                    pre_gap_header->prev = curr_index;
                    pre_gap_header->next = 0; /* free blocks dont have a defined next index */
                }
                else
                {
                    /* no pre-gap, current block points directly to allocation */
                    curr_header->next = alloc_index;
                }
                
                last_pos = alloc_index;
                logalloc_pool_cap += blocksize;
                
                /* create the new allocated block */
                gap_header = CONV_IDX_TO_ADDR(alloc_index);
                gap_header->magic = MAGIC_NUMBER;
                gap_header->prev = (pre_gap_size > 0) ? gap_index : curr_index;
                gap_header->next = curr_index_next;
                
                /* handle post-gap if exists */
                if (post_gap_size > 0)
                {
                    /* create a free block after the allocation */
                    uint32 post_gap_index = alloc_index + blocksize;
                    curr_header_next->prev = post_gap_index;
                    logalloc_block_header* post_gap_header = CONV_IDX_TO_ADDR(post_gap_index);
                    post_gap_header->magic = MAGIC_NUMBER_FREE;
                    post_gap_header->prev = alloc_index;
                    post_gap_header->next = 0; /* free blocks dont have a defined next index */
                }
                else
                {
                    /* perfect fit, update next block's prev */
                    curr_header_next->prev = alloc_index;
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
#endif

void* logalloc_realloc_memory(void* ptr, uint32 size)
{
    void* temp = logalloc_allocate_memory(size);
    memcpy(temp, ptr, size);

    logalloc_free_memory(ptr);
    return temp;
}

/* reposition the last position pointer in the logalloc pool */
void logalloc_reposition_last_pos(void* ptr)
{
    last_pos = ((uint32*)ptr - logalloc_pool) - (sizeof(logalloc_block_header) / sizeof(uint32)); /* get header index from data pointer */
#ifdef RELATIVE_INDEXING
    m_assert(RELADR_MAGIC_NUMBER(last_pos) == MAGIC_NUMBER, "you are passing an invalid pointer. "
        "last_pos must point to a valid allocated block in the logalloc pool");
#else
    m_assert(CONV_IDX_TO_ADDR(last_pos)->magic == MAGIC_NUMBER, "you are passing an invalid pointer. "
        "last_pos must point to a valid allocated block in the logalloc pool");
#endif
}

void logalloc_dump_pool()
{
    FILE* outfile = fopen("logalloc_dump.bin", "wb");
    
    m_assert(outfile != NULL, "failed to open logalloc_dump.bin for writing");

    /* write the entire logalloc_pool buffer to file */
    size_t written = fwrite(logalloc_pool, sizeof(uint32), MAX_POOL_SIZE / sizeof(uint32), outfile);

    m_assert(written == (MAX_POOL_SIZE / sizeof(uint32)), "failed to write to logalloc_dump.bin");

    fclose(outfile);

    printf("logalloc pool dumped to logalloc_dump.bin\n");
}


