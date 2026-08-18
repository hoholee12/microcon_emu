#include "EmuPool.hpp"
#include <vector>
#include <ctime>
#include <stdio.h>

/* EmuPool - Custom Memory Allocator for Emulation Structures
 * 
 * OVERVIEW:
 * Linked-list based allocator with gap detection, coalescing, alignment support, and wraparound sentinels.
 * Supports two modes: RELATIVE_INDEXING (space-efficient, 24-bit offsets) and absolute indexing (32-bit pointers).
 * 
 * 
 * HEADER STRUCTURE
 * 
 * 
 * RELATIVE_INDEXING mode (8 bytes):
 * - 2 words with position-based XOR encoding for corruption detection
 * - Magic number: 0xAA (allocated) or 0xCC (freed)
 * - Prev offset: 24-bit signed offset to previous block
 * - Next offset: 24-bit signed offset to next block
 * - Encoding: firstword = (~index) XOR data, secondword = index XOR data
 * - Offsets wrap around at pool boundaries automatically
 * 
 * Absolute indexing mode (12 bytes):
 * - Magic: 32-bit (0xAAAAAAAA allocated, 0xCCCCCCCC freed)
 * - Prev: 32-bit absolute index to previous block
 * - Next: 32-bit absolute index to next block
 * 
 * 
 * ALLOCATION STRUCTURE
 * 
 * 
 * Each allocation consists of:
 * [Header][Data...]
 * 
 * Returned pointer points to Data, not Header.
 * Header can be accessed by subtracting header_size from returned pointer.
 * 
 * 
 * SENTINEL BLOCKS (Wraparound Support)
 * 
 * 
 * Multiple sentinel blocks are distributed across the pool at MEDIAN_SENTINEL_DISTANCE intervals.
 * Each sentinel consists of an allocated block followed by a gap (free) block.
 * 
 * Structure:
 * - First sentinel at index 0
 * - Middle sentinels every MEDIAN_SENTINEL_DISTANCE words
 * - Last sentinel at last_alloc_pos = (MAX_POOL_SIZE - header_size) / sizeof(uint32)
 * - Each sentinel's next pointer points to the next sentinel (skipping the gap)
 * 
 * This...:
 * - Enables circular wraparound search from any starting position
 * - Search continues until returning to start position (one full cycle)
 * 
 * For small pools (MEDIAN_SENTINEL_DISTANCE >= last_alloc_pos):
 * - Only two sentinels: one at index 0, one at last_alloc_pos
 * 
 * 
 * GAP DETECTION
 * 
 * 
 * Freed blocks create "gaps" in the linked list:
 * 1. When block B (between A and C) is freed:
 *    - Block A's next pointer updated to point to C (skipping B)
 *    - Block B's magic set to 0xCC (freed marker)
 *    - Block B's prev pointer PRESERVED (needed for coalescing)
 *    - Block B's next pointer cleared to 0
 *    - Block C's prev pointer keeps pointing to B (gap marker)
 * 
 * 2. During allocation traversal:
 *    - If curr_block.prev points to a freed block, a gap exists
 *    - Gap location: curr_block.prev
 *    - Gap size: curr_index - gap_index
 *    - If gap is large enough, allocate there
 * 
 * 
 * COALESCING (Combining Adjacent Freed Blocks)
 * 
 * 
 * During deallocation, adjacent freed blocks are merged:
 * 
 * 1. Walk backwards using preserved prev pointers through freed blocks
 *    until finding an allocated block (magic == 0xAA/0xAAAAAAAA)
 * 
 * 2. Update that allocated block's next pointer to skip all consecutive
 *    freed blocks including the one being freed
 * 
 * 3. Example: A -> [freed B] -> [freed C] -> [to-free D] -> E
 *    Result:  A -> E (with large gap covering B, C, D)
 * 
 * This CANNOT reduce external fragmentation, but it avoids traversing multiple freed blocks while searching for gaps.
 * 
 * 
 * ALIGNMENT SUPPORT (Pre-Gap Poking)
 * 
 * 
 * When allocation requires specific alignment:
 * 
 * 1. Find a gap large enough to contain:
 *    - Pre-gap free block (if needed for alignment)
 *    - Aligned allocation
 *    - Post-gap free block (if space remains)
 * 
 * 2. Calculate aligned position within gap:
 *    aligned_index = ((gap_index + header_size + align_words - 1) & ~(align_words - 1)) - header_size
 * 
 * 3. If gap_index < aligned_index:
 *    - Create "pre-gap" free block at original gap_index
 *    - Allocate at aligned_index
 *    - Update links to include pre-gap in free chain
 * 
 * 4. Example (16-byte alignment required):
 *    Gap at index 0x1234, size 0x100
 *    Aligned position: 0x1238 (data at 0x123A after header)
 *    Result: [pre-gap free: 0x1234-0x1237][allocated: 0x1238-...][post-gap if needed]
 * 
 * 
 * PERFORMANCE OPTIMIZATIONS
 * 
 * 
 * last_pos caching:
 * - Tracks the last successful allocation position
 * - Next allocation starts searching from last_pos (for locality principle)
 * - On deallocation, last_pos rewinds to the previous allocated block
 * 
 * last_pos_perf_penalty:
 * - Incremented when last_pos validation fails (corruption or invalid)
 * 
 * 
 * MEMORY LAYOUT EXAMPLE (Absolute Mode)
 * 
 * 
 * Three blocks allocated, middle one freed:
 * 
 * Block A (allocated):
 * [0x0000] magic=0xAAAAAAAA, prev=last_alloc_pos, next=0x2003
 * [0x0003] ... data ...
 * 
 * Block B (freed):
 * [0x2003] magic=0xCCCCCCCC, prev=0x0000, next=0x0000
 * [0x2006] ... (freed space) ...
 * 
 * Block C (allocated):
 * [0x4006] magic=0xAAAAAAAA, prev=0x2003 (gap marker), next=last_alloc_pos
 * [0x4009] ... data ...
 * 
 * Sentinel at end:
 * [last_alloc_pos] magic=0xAAAAAAAA, prev=0x4006, next=0x0000 (wraps to start)
 * 
 * Forward chain: A -> C -> Sentinel -> (wrap) A
 * Gap detected: C.prev (0x2003) points to freed block B
 */

uint32 *logalloc_pool;
uint32 last_pos_perf_penalty = 0;    /* performance metric for last_pos misses */
uint32 last_pos = 0;    /* last position for better performance */
uint32 last_alloc_pos = (MAX_POOL_SIZE - sizeof(logalloc_block_header)) / sizeof(uint32);
uint32 logalloc_pool_cap = 0;
#ifdef USE_DEBUG_BLOCK
logalloc_debug_block* debug_block; /* pointer to debug block for debugging purposes */
uint32 debug_block_init = 0;
#endif
uint32 dump_count = 0; /* track how many times the pool has been dumped */

#ifdef RELATIVE_INDEXING
void logalloc_init()
{
    uint32 prev_pos = 0;
    uint32 prev_gap_pos = 0;
    uint32 curr_pos = 0;
    uint32 curr_gap_pos = 0;

    uint32 blocksize = sizeof(logalloc_block_header) / sizeof(uint32); /* the beginning and the end */
    prev_gap_pos = blocksize; /* the first gap block is after the first sentinel block */

    logalloc_pool = (uint32*)malloc(MAX_POOL_SIZE * sizeof(uint32));
    memset(logalloc_pool, 0, MAX_POOL_SIZE * sizeof(uint32));
    last_pos = 0;
    logalloc_pool_cap = 0;

    if (last_alloc_pos <= MEDIAN_SENTINEL_DISTANCE)
    {
        /* allocate the first block for wraparound sentinel */
        RELADR_HEAD_UPDATE(0, blocksize, last_alloc_pos); /* first: prevoffset underflows to last_alloc, nextoffset skips gap to last_alloc */
        /* create a free block in the middle to represent the gap for gap detection */
        RELADR_HEAD_UPDATE_FREE(blocksize, blocksize); /* gap: prevoffset to first sentinel (not in forward chain) */
        RELADR_HEAD_UPDATE(last_alloc_pos, last_alloc_pos - blocksize, blocksize); /* last: prevoffset to gap, nextoffset wraps to 0 */
    }
    else
    {
        RELADR_HEAD_UPDATE(0, blocksize, MEDIAN_SENTINEL_DISTANCE); /* first block for wraparound */
        RELADR_HEAD_UPDATE_FREE(blocksize, blocksize);
        while(prev_pos + MEDIAN_SENTINEL_DISTANCE < last_alloc_pos)
        {
            curr_pos = prev_pos + MEDIAN_SENTINEL_DISTANCE;
            curr_gap_pos = prev_gap_pos + MEDIAN_SENTINEL_DISTANCE;

            RELADR_HEAD_UPDATE(curr_pos, MEDIAN_SENTINEL_DISTANCE - blocksize, MEDIAN_SENTINEL_DISTANCE);
            RELADR_HEAD_UPDATE_FREE(curr_gap_pos, blocksize);

            prev_pos = curr_pos;
            prev_gap_pos = curr_gap_pos;
            logalloc_pool_cap += blocksize;
        }

        /* last block - repeat of the final iter for last_alloc_pos offset calc */
        RELADR_HEAD_UPDATE(curr_pos, MEDIAN_SENTINEL_DISTANCE - blocksize, last_alloc_pos - curr_pos); /* one last middle sentinel and a gap */
        RELADR_HEAD_UPDATE_FREE(curr_gap_pos, blocksize);

        RELADR_HEAD_UPDATE(last_alloc_pos, (curr_gap_pos == last_alloc_pos) ? blocksize : (last_alloc_pos - curr_gap_pos), blocksize); /* end block for wraparound */
    }

    last_pos = 0;
    logalloc_pool_cap += blocksize * 2;
    last_pos_perf_penalty = 0;

#ifdef USE_DEBUG_BLOCK
    /* allocate a debug block for debugging purposes */
    debug_block = (logalloc_debug_block*)logalloc_allocate_memory(sizeof(logalloc_debug_block), 0);
    debug_block->debug_magic = DEBUG_MAGIC_NUMBER;
    debug_block->debug_last_pos = last_pos;
    debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
    debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
    debug_block->debug_logalloc_totalsize = MAX_POOL_SIZE / sizeof(uint32);
    debug_block_init = 1;
#endif
}
#else
void logalloc_init()
{
    logalloc_block_header* curr_header;
    uint32 blocksize = sizeof(logalloc_block_header) / sizeof(uint32); /* the beginning and the end */

    logalloc_pool = (uint32*)malloc(MAX_POOL_SIZE * sizeof(uint32));
    memset(logalloc_pool, 0, MAX_POOL_SIZE * sizeof(uint32));
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
    logalloc_pool_cap = blocksize * 2;

    last_pos_perf_penalty = 0;
    
#ifdef USE_DEBUG_BLOCK
    /* allocate a debug block for debugging purposes */
    debug_block = (logalloc_debug_block*)logalloc_allocate_memory(sizeof(logalloc_debug_block), 0);
    debug_block->debug_magic = DEBUG_MAGIC_NUMBER;
    debug_block->debug_last_pos = last_pos;
    debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
    debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
    debug_block->debug_logalloc_totalsize = MAX_POOL_SIZE / sizeof(uint32);
    debug_block_init = 1;
#endif
}
#endif

/* for calloc */
void* logalloc_allocate_clear_memory(uint32 size, uint32 align_bytes)
{
    uint32* ptr = NULL;
    ptr = (uint32*)logalloc_allocate_memory(size, align_bytes);
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

#ifdef USE_DEBUG_BLOCK
    /* update debug block */
    if (debug_block_init == 1)
    {
        debug_block->debug_last_pos = last_pos;
        debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
        debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
    }
#endif
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

#ifdef USE_DEBUG_BLOCK
    /* update debug block */
    if (debug_block_init == 1)
    {
        debug_block->debug_last_pos = last_pos;
        debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
        debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
    }
#endif
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
 * - ex) assuming header is 2 words (relative indexing)
 * aligned_index = (current_index + (align_bytes / sizeof(uint32) - 1)) & ~(align_bytes / sizeof(uint32) - 1)
 * ex) current_index = 0x1234 (index after header = 0x1236), align_bytes = 16
 * aligned_index = (0x1236 + (16 / 4 - 1)) & ~(16 / 4 - 1) = (0x1236 + 3) & ~3 = 0x1239 & ~3 = 0x1238 (snap to next aligned index)
 * ex) current_index = 0x1232 (index after header = 0x1234), align_bytes = 16
 * aligned_index = (0x1234 + 3) & ~3 = 0x1237 & ~3 = 0x1234 (already aligned)
 * 
    if (align_bytes > 0)
    {
        uint32 header_words = sizeof(logalloc_block_header) / sizeof(uint32);
        uint32 align_words = align_bytes / sizeof(uint32);
        uint32 gap_data_index = gap_index + header_words;
        uint32 aligned_data_index = (gap_data_index + (align_words - 1)) & ~(align_words - 1);
        alloc_index = aligned_data_index - header_words;
    }
 * 
 */
#ifdef RELATIVE_INDEXING
void* logalloc_allocate_memory(uint32 bytecount, uint32 align_bytes)
{
    uint32 curr_index = 0; /* must always point to header magic */
    uint32 blocksize = ((uint32)bytecount + sizeof(logalloc_block_header)) / sizeof(uint32); /* blocksize must be in word units */
    uint32 curr_index_prev, curr_index_next;
    uint32 currsize = 0;
    uint32 gap_index = 0;
    uint32 gapsize = 0;
    /* only used on pre_gap_poking */
    uint32 pre_gap_index = 0;
    uint32 pre_gapsize = 0;
    uint32 middle_alloc_index = 0;
    uint32 pre_gap_poking_required = 0;

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
            gap_index = RELADR_PREV_IDX(curr_index_next);
            gapsize = curr_index_next - gap_index;
            currsize = gap_index - curr_index;
            /* only used on pre_gap_poking */
            pre_gap_index = 0;
            pre_gapsize = 0;
            middle_alloc_index = 0;
            pre_gap_poking_required = 0;

            /* shit is too fucking complicated
             * pre-gap-poking only when really required
             * hide the pre-gap from the main logic */
            if (align_bytes > 0)
            {
                uint32 header_words = sizeof(logalloc_block_header) / sizeof(uint32);
                uint32 align_words = align_bytes / sizeof(uint32);
                uint32 gap_data_index = gap_index + header_words;
                uint32 aligned_data_index = (gap_data_index + (align_words - 1)) & ~(align_words - 1);
                m_assert(INLINE_HEADER_ALIGN_CHECK(aligned_data_index - header_words) == LOGALLOC_OK, "you cannot align to a value that is not a base of headersize");
                if (gap_index < (aligned_data_index - header_words))
                {
                    middle_alloc_index = (aligned_data_index - header_words);
                    pre_gap_index = gap_index;
                    pre_gapsize = middle_alloc_index - gap_index;
                    /* search the whole gap for possible aligned allocation
                     * redo the gap index and gapsize */
                    gap_index = middle_alloc_index; /* main logic will make the gap index the new alloc index */
                    gapsize = 0; /* set gapsize to 0 in case gap index search fails so that main logic will not run */
                    while (gap_index < curr_index_next)
                    {
                        if ((curr_index_next > gap_index) /* gap after pre-gap subtracted is still enough for allocation */
                            && (gap_index >= pre_gap_index + header_words)) /* new gap index is not overlapping the old gap index free block */
                        {
                            pre_gapsize = gap_index - pre_gap_index;
                            gapsize = curr_index_next - gap_index;
                            currsize = gap_index - curr_index;
                            pre_gap_poking_required = 1;
                            break;
                        }
                        gap_index += align_words; /* try next aligned index */
                    }
                }
            }

            if (gapsize >= blocksize)
            {
                /* we have enough space to insert a new block here */

                if (pre_gap_poking_required != 0)
                {
                    /* we need to poke it(new free block in otherwise nonexistant area) */
                    RELADR_HEAD_UPDATE_FREE(pre_gap_index, pre_gap_index - curr_index);
                }

                RELADR_HEAD_UPDATE(curr_index, RELADR_PREV_OFFSET(curr_index), currsize); /* update current block's next to point to new block */
                last_pos = gap_index;
                logalloc_pool_cap += blocksize;
                /* new block - point prev to pre_gap_index if its align issued */
                RELADR_HEAD_UPDATE(gap_index, (pre_gap_poking_required != 0) ? pre_gapsize : currsize, gapsize);

                if (gapsize == blocksize)
                {
                    /* perfect fit, we can just update the next block's prev to point to new block */
                    RELADR_HEAD_UPDATE(curr_index_next, gapsize, RELADR_NEXT_OFFSET(curr_index_next));
                }
                else
                {
                    /* we can fit more, we need to update the next block's prev to point to new block's next
                     * - we plant gap logic for future allocs here */
                    uint32 post_gap_index = gap_index + blocksize;
                    uint32 post_gapsize = gapsize - blocksize;
                    RELADR_HEAD_UPDATE(curr_index_next, post_gapsize, RELADR_NEXT_OFFSET(curr_index_next));

                    /* if we were to deallocate, make it a lone island, 
                     * and then try reallocating the left side with a smaller block; */
                    /* we need to plant free magic here */
                    RELADR_HEAD_UPDATE_FREE(post_gap_index, blocksize); /* new gap block */
                }

#ifdef USE_DEBUG_BLOCK
                /* update debug block */
                if (debug_block_init == 1)
                {
                    debug_block->debug_last_pos = last_pos;
                    debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
                    debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
                }
#endif

                return CONV_ADDR_TO_BODY(CONV_IDX_TO_ADDR(gap_index)); /* return data area */
            }
            else
            {
                /* gap too small, keep looking */
            }
        }
        else
        {
            if (curr_index_prev == 0x0)
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

#ifdef USE_DEBUG_BLOCK
    /* update debug block */
    if (debug_block_init == 1)
    {
        debug_block->debug_last_pos = last_pos;
        debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
        debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
    }
#endif
}
#else
void* logalloc_allocate_memory(uint32 bytecount, uint32 align_bytes)
{
    uint32 curr_index = 0; /* must always point to header magic */
    uint32 blocksize = ((uint32)bytecount + sizeof(logalloc_block_header)) / sizeof(uint32); /* blocksize must be in word units */
    uint32 curr_index_prev, curr_index_next;
    logalloc_block_header *curr_header, *curr_header_prev, *curr_header_next, *gap_header;
    uint32 gap_index = 0;
    uint32 gapsize = 0;
    /* only used on pre_gap_poking */
    uint32 pre_gap_index = 0;
    uint32 pre_gapsize = 0;
    uint32 middle_alloc_index = 0;
    uint32 pre_gap_poking_required = 0;

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
            gap_index = curr_header_next->prev;
            gapsize = curr_index_next - gap_index;
            /* only used on pre_gap_poking */
            pre_gap_index = 0;
            pre_gapsize = 0;
            middle_alloc_index = 0;
            pre_gap_poking_required = 0;

            /* shit is too fucking complicated
             * pre-gap-poking only when really required
             * hide the pre-gap from the main logic */
            if (align_bytes > 0)
            {
                uint32 header_words = sizeof(logalloc_block_header) / sizeof(uint32);
                uint32 align_words = align_bytes / sizeof(uint32);
                uint32 gap_data_index = gap_index + header_words;
                uint32 aligned_data_index = (gap_data_index + (align_words - 1)) & ~(align_words - 1);
                m_assert(INLINE_HEADER_ALIGN_CHECK(aligned_data_index - header_words) == LOGALLOC_OK, "you cannot align to a value that is not a base of headersize");
                if (gap_index < (aligned_data_index - header_words))
                {
                    middle_alloc_index = (aligned_data_index - header_words);
                    pre_gap_index = gap_index;
                    pre_gapsize = middle_alloc_index - gap_index;
                    /* search the whole gap for possible aligned allocation
                     * redo the gap index and gapsize */
                    gap_index = middle_alloc_index; /* main logic will make the gap index the new alloc index */
                    gapsize = 0; /* set gapsize to 0 in case gap index search fails so that main logic will not run */
                    while (gap_index < curr_index_next)
                    {
                        if ((curr_index_next > gap_index) /* gap after pre-gap subtracted is still enough for allocation */
                            && (gap_index >= pre_gap_index + header_words)) /* new gap index is not overlapping the old gap index free block */
                        {
                            pre_gapsize = gap_index - pre_gap_index;
                            gapsize = curr_index_next - gap_index;
                            pre_gap_poking_required = 1;
                            break;
                        }
                        gap_index += align_words; /* try next aligned index */
                    }
                }
            }

            /* main logic */
            if (gapsize >= blocksize)
            {
                /* we have enough space to insert a new block here */

                if (pre_gap_poking_required != 0)
                {
                    /* we need to poke it(new free block in otherwise nonexistant area) */
                    logalloc_block_header* pre_gap_header = CONV_IDX_TO_ADDR(pre_gap_index);
                    pre_gap_header->magic = MAGIC_NUMBER_FREE;
                    pre_gap_header->prev = curr_index;
                    pre_gap_header->next = 0; /* free blocks dont have a defined next index */
                    /* curr_index to point to pre_gap_index if pre-gap poking is required */
                    curr_index = pre_gap_index;
                }

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
                    post_gap_header->next = 0; /* free blocks dont have a defined next index */
                }

#ifdef USE_DEBUG_BLOCK
            /* update debug block */
            if (debug_block_init == 1)
            {
                debug_block->debug_last_pos = last_pos;
                debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
                debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
            }
#endif

                return CONV_ADDR_TO_BODY(gap_header); /* return data area */
            }
            else
            {
                /* gap too small, keep looking */
            }
        }
        else
        {
            if (curr_index_prev == 0x0)
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
    
#ifdef USE_DEBUG_BLOCK
    /* update debug block */
    if (debug_block_init == 1)
    {
        debug_block->debug_last_pos = last_pos;
        debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
        debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
    }
#endif
}
#endif

#ifdef RELATIVE_INDEXING
uint32 logalloc_expand_datablock(void* ptr, uint32 newsize)
{
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - (sizeof(logalloc_block_header) / sizeof(uint32)); /* get header index from data pointer */
    m_assert(RELADR_MAGIC_NUMBER(baseindex) == MAGIC_NUMBER, "memory corruption, or you are passing an invalid pointer");
    /* get oldsize and gapsize */
    uint32 nextblock_startidx = RELADR_NEXT_IDX(baseindex);
    uint32 nextblock_startoffset = RELADR_NEXT_OFFSET(baseindex);
    uint32 old_gap_index = RELADR_PREV_IDX(nextblock_startidx);
    uint32 old_gap_offset = RELADR_PREV_OFFSET(nextblock_startidx);
    uint32 gapsize = (old_gap_index == baseindex) ? 0 : (nextblock_startidx - old_gap_index);
    uint32 oldsize = (gapsize == 0) ? (nextblock_startidx - baseindex) : (old_gap_index - baseindex);
    uint32 subtract_flag = 0;
    uint32 appendsize = 0;
    uint32 post_gap_offset = 0;
    uint32 post_gap_index = 0;

    if (newsize > oldsize)
    {
        appendsize = newsize - oldsize;
    }
    else if (newsize < oldsize)
    {
        subtract_flag = 1;
        appendsize = oldsize - newsize;
    }
    else /* newsize == oldsize */
    {
        return LOGALLOC_OK; /* do nothing */
    }
    
    /* check if it can be expanded */
    if (gapsize > appendsize)
    {
        if (subtract_flag == 0)
        {
            /* increase prev index to new gap position */
            post_gap_offset = old_gap_offset - appendsize;
            post_gap_index = old_gap_index + appendsize;
            if (INLINE_HEADER_ALIGN_CHECK(post_gap_index) == LOGALLOC_ERROR_NOT_HEADER_ALIGNED)
            {
                return LOGALLOC_ERROR_NOT_ENOUGH_GAP;
            }
            RELADR_HEAD_UPDATE(nextblock_startidx, post_gap_offset, RELADR_NEXT_OFFSET(nextblock_startidx));
            RELADR_HEAD_UPDATE_FREE(post_gap_index, post_gap_index - baseindex); /* new gap block */
            logalloc_pool_cap += appendsize;
        }
        else
        {
            /* shrink instead */
            post_gap_offset = old_gap_offset + appendsize;
            post_gap_index = old_gap_index - appendsize;
            if (INLINE_HEADER_ALIGN_CHECK(post_gap_index) == LOGALLOC_ERROR_NOT_HEADER_ALIGNED)
            {
                return LOGALLOC_ERROR_NOT_ENOUGH_GAP;
            }
            RELADR_HEAD_UPDATE(nextblock_startidx, post_gap_offset, RELADR_NEXT_OFFSET(nextblock_startidx));
            RELADR_HEAD_UPDATE_FREE(post_gap_index, post_gap_index - baseindex); /* new gap block */
            logalloc_pool_cap -= appendsize;
        }
    }
    else if (gapsize == appendsize)
    {
        /* perfect fit, we can just update the next block's prev to point to new block */
        RELADR_HEAD_UPDATE(nextblock_startidx, nextblock_startoffset, RELADR_NEXT_OFFSET(nextblock_startidx));
        logalloc_pool_cap += appendsize;
    }
    else /* gapsize < appendsize - return error */
    {
        return LOGALLOC_ERROR_NOT_ENOUGH_GAP;
    }

#ifdef USE_DEBUG_BLOCK
    /* update debug block */
    if (debug_block_init == 1)
    {
        debug_block->debug_last_pos = last_pos;
        debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
        debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
    }
#endif
    return LOGALLOC_OK;
}
#else
uint32 logalloc_expand_datablock(void* ptr, uint32 newsize)
{
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - (sizeof(logalloc_block_header) / sizeof(uint32)); /* get header index from data pointer */
    logalloc_block_header* curr_header = CONV_IDX_TO_ADDR(baseindex);
    m_assert(curr_header->magic == MAGIC_NUMBER, "memory corruption, or you are passing an invalid pointer");
    /* get oldsize and gapsize */
    uint32 nextblock_startidx = curr_header->next;
    uint32 old_gap_index = CONV_IDX_TO_ADDR(nextblock_startidx)->prev;
    uint32 gapsize = (old_gap_index == baseindex) ? 0 : (nextblock_startidx - old_gap_index);
    uint32 oldsize = (gapsize == 0) ? (nextblock_startidx - baseindex) : (old_gap_index - baseindex);
    uint32 subtract_flag = 0;
    uint32 appendsize = 0;
    uint32 post_gap_index = 0;

    if (newsize > oldsize)
    {
        appendsize = newsize - oldsize;
    }
    else if (newsize < oldsize)
    {
        subtract_flag = 1;
        appendsize = oldsize - newsize;
    }
    else /* newsize == oldsize */
    {
        return LOGALLOC_OK; /* do nothing */
    }
    
    /* check if it can be expanded */
    if (gapsize > appendsize)
    {
        if (subtract_flag == 0)
        {
            /* increase prev index to new gap position */
            post_gap_index = old_gap_index + appendsize;
            if (INLINE_HEADER_ALIGN_CHECK(post_gap_index) == LOGALLOC_ERROR_NOT_HEADER_ALIGNED)
            {
                return LOGALLOC_ERROR_NOT_ENOUGH_GAP;
            }
            CONV_IDX_TO_ADDR(nextblock_startidx)->prev = post_gap_index;
            logalloc_block_header* post_gap_header = CONV_IDX_TO_ADDR(post_gap_index);
            post_gap_header->magic = MAGIC_NUMBER_FREE;
            post_gap_header->prev = baseindex;
            post_gap_header->next = 0; /* free blocks dont have a defined next index */
            logalloc_pool_cap += appendsize;
        }
        else
        {
            /* shrink instead */
            post_gap_index = old_gap_index - appendsize;
            if (INLINE_HEADER_ALIGN_CHECK(post_gap_index) == LOGALLOC_ERROR_NOT_HEADER_ALIGNED)
            {
                return LOGALLOC_ERROR_NOT_ENOUGH_GAP;
            }
            CONV_IDX_TO_ADDR(nextblock_startidx)->prev = post_gap_index;
            logalloc_block_header* post_gap_header = CONV_IDX_TO_ADDR(post_gap_index);
            post_gap_header->magic = MAGIC_NUMBER_FREE;
            post_gap_header->prev = baseindex;
            post_gap_header->next = 0; /* free blocks dont have a defined next index */
            logalloc_pool_cap -= appendsize;
        }
    }
    else if (gapsize == appendsize)
    {
        /* update the next blocks prev to point to prev alloc block (to highlight no gap) */
        CONV_IDX_TO_ADDR(nextblock_startidx)->prev = baseindex;
        logalloc_pool_cap += appendsize;
    }
    else /* gapsize < appendsize - return error */
    {
        return LOGALLOC_ERROR_NOT_ENOUGH_GAP;
    }

#ifdef USE_DEBUG_BLOCK
    /* update debug block */
    if (debug_block_init == 1)
    {
        debug_block->debug_last_pos = last_pos;
        debug_block->debug_last_pos_perf_penalty = last_pos_perf_penalty;
        debug_block->debug_logalloc_pool_cap = logalloc_pool_cap;
    }
#endif
    return LOGALLOC_OK;
}
#endif

#ifdef RELATIVE_INDEXING
uint32 logalloc_move_zero_datablock(void* ptr, uint32 newindex)
{
    uint32 headersize = sizeof(logalloc_block_header) / sizeof(uint32);
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - headersize; /* get header index from data pointer */
    m_assert(RELADR_MAGIC_NUMBER(baseindex) == MAGIC_NUMBER, "memory corruption, or you are passing an invalid pointer");

    m_assert(INLINE_HEADER_ALIGN_CHECK(newindex) == LOGALLOC_OK, "you cannot move to an index that is not aligned with headersize");
    
    uint32 nextblock_startidx = RELADR_NEXT_IDX(baseindex);
    uint32 prevblock_startidx = RELADR_PREV_IDX(baseindex);
    uint32 possible_range_startidx = 0;
    uint32 possible_range_endidx = 0;
    uint32 left_section_gap_exists = 0;
    uint32 rightside_gap_index = newindex + headersize;
    
    /* check the data size first
     * if its not 0, its an error */
    m_assert((RELADR_PREV_IDX(nextblock_startidx) - baseindex) == headersize, "move_zero_datablock expects an allocated block with data size of 0 (usually median sentinel)");
    
    /* check gap from left and right
     * we can only move the block as far as gap range goes */

    if (RELADR_MAGIC_NUMBER(prevblock_startidx) == MAGIC_NUMBER_FREE)
    {
        possible_range_startidx = prevblock_startidx;
        left_section_gap_exists = 1;
    }
    else
    {
        possible_range_startidx = baseindex;
    }

    if (RELADR_MAGIC_NUMBER(RELADR_PREV_IDX(nextblock_startidx)) == MAGIC_NUMBER_FREE)
    {
        possible_range_endidx = nextblock_startidx - headersize;
    }
    else
    {
        possible_range_endidx = baseindex;
    }
    
    /* validate newindex */
    if (baseindex == newindex)
    {
        /* do nothing */
    }
    else if ((possible_range_startidx <= newindex) && (possible_range_endidx >= newindex))
    {
        if (left_section_gap_exists == 1)
        {
            uint32 prev_prevblock_startidx = RELADR_PREV_IDX(prevblock_startidx);
            RELADR_HEAD_UPDATE(prev_prevblock_startidx, RELADR_PREV_OFFSET(prev_prevblock_startidx), newindex - prev_prevblock_startidx);
        }
        else
        {
            RELADR_HEAD_UPDATE(prevblock_startidx, RELADR_PREV_OFFSET(prevblock_startidx), newindex - prevblock_startidx);
        }
        RELADR_HEAD_UPDATE(newindex, newindex - prevblock_startidx, nextblock_startidx - newindex);
        
        /* put rightside free block if needed */
        if (possible_range_endidx >= rightside_gap_index)
        {
            RELADR_HEAD_UPDATE_FREE(rightside_gap_index, rightside_gap_index - newindex);
            RELADR_HEAD_UPDATE(nextblock_startidx, nextblock_startidx - rightside_gap_index, RELADR_NEXT_OFFSET(nextblock_startidx));
        }
        else
        {
            RELADR_HEAD_UPDATE(nextblock_startidx, nextblock_startidx - newindex, RELADR_NEXT_OFFSET(nextblock_startidx));
        }
    }
    else
    {
        return LOGALLOC_ERROR_NOT_ENOUGH_GAP;
    }

    return LOGALLOC_OK;
}
#else
uint32 logalloc_move_zero_datablock(void* ptr, uint32 newindex)
{
    uint32 headersize = sizeof(logalloc_block_header) / sizeof(uint32);
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - headersize; /* get header index from data pointer */
    logalloc_block_header* curr_header = CONV_IDX_TO_ADDR(baseindex);
    m_assert(curr_header->magic == MAGIC_NUMBER, "memory corruption, or you are passing an invalid pointer");

    m_assert(INLINE_HEADER_ALIGN_CHECK(newindex) == LOGALLOC_OK, "you cannot move to an index that is not aligned with headersize");
    
    uint32 nextblock_startidx = curr_header->next;
    uint32 prevblock_startidx = curr_header->prev;
    uint32 possible_range_startidx = 0;
    uint32 possible_range_endidx = 0;
    logalloc_block_header* prevblock_header = CONV_IDX_TO_ADDR(prevblock_startidx);
    logalloc_block_header* nextblock_header = CONV_IDX_TO_ADDR(nextblock_startidx);
    logalloc_block_header* newblock_header = CONV_IDX_TO_ADDR(newindex);
    uint32 left_section_gap_exists = 0;
    uint32 rightside_gap_index = newindex + headersize;
    
    /* check the data size first
     * if its not 0, its an error */
    m_assert((nextblock_header->prev - baseindex) == headersize, "move_zero_datablock expects an allocated block with data size of 0 (usually median sentinel)");
    
    /* check gap from left and right
     * we can only move the block as far as gap range goes */

    if (CONV_IDX_TO_ADDR(prevblock_startidx)->magic == MAGIC_NUMBER_FREE)
    {
        possible_range_startidx = prevblock_startidx;
        left_section_gap_exists = 1;
    }
    else
    {
        possible_range_startidx = baseindex;
    }

    if (CONV_IDX_TO_ADDR(CONV_IDX_TO_ADDR(nextblock_startidx)->prev)->magic == MAGIC_NUMBER_FREE)
    {
        possible_range_endidx = nextblock_startidx - headersize;
    }
    else
    {
        possible_range_endidx = baseindex;
    }
    
    /* validate newindex */
    if (baseindex == newindex)
    {
        /* do nothing */
    }
    else if ((possible_range_startidx <= newindex) && (possible_range_endidx >= newindex))
    {
        if (left_section_gap_exists == 1)
        {
            CONV_IDX_TO_ADDR(prevblock_header->prev)->next = newindex;
        }
        else
        {
            prevblock_header->next = newindex;
        }
        newblock_header->magic = MAGIC_NUMBER;
        newblock_header->prev = prevblock_startidx; /* points to either gap or legit alloc block */
        newblock_header->next = nextblock_startidx;
        /* put rightside free block if needed */
        if (possible_range_endidx >= rightside_gap_index)
        {
            logalloc_block_header* rightside_gap_header = CONV_IDX_TO_ADDR(rightside_gap_index);
            rightside_gap_header->magic = MAGIC_NUMBER_FREE;
            rightside_gap_header->prev = newindex;
            rightside_gap_header->next = 0;
            nextblock_header->prev = rightside_gap_index;
        }
        else
        {
            nextblock_header->prev = newindex;
        }
    }
    else
    {
        return LOGALLOC_ERROR_NOT_ENOUGH_GAP;
    }

    return LOGALLOC_OK;

}
#endif

#ifdef RELATIVE_INDEXING
uint32 logalloc_get_datablock_size(void* ptr)
{
    uint32 headersize = sizeof(logalloc_block_header) / sizeof(uint32);
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - headersize; /* get header index from data pointer */
    uint32 nextblock_startidx = RELADR_NEXT_IDX(baseindex);
    if (RELADR_PREV_IDX(nextblock_startidx) == baseindex)
    {
        /* alloc -> alloc */
        return nextblock_startidx - baseindex - headersize;
    }
    else
    {
        /* alloc -> gap -> alloc */
        return RELADR_PREV_IDX(nextblock_startidx) - baseindex - headersize;
    }
}
#else
uint32 logalloc_get_datablock_size(void* ptr)
{
    uint32 headersize = sizeof(logalloc_block_header) / sizeof(uint32);
    uint32 baseindex = ((uint32*)ptr - logalloc_pool) - headersize; /* get header index from data pointer */
    logalloc_block_header* header = CONV_IDX_TO_ADDR(baseindex);
    if (CONV_IDX_TO_ADDR(header->next)->prev == baseindex)
    {
        /* alloc -> alloc */
        return header->next - baseindex - headersize;
    }
    else
    {
        /* alloc -> gap -> alloc */
        return CONV_IDX_TO_ADDR(header->next)->prev - baseindex - headersize;
    }
}
#endif

void* logalloc_realloc_memory(void* ptr, uint32 size, uint32 align_bytes)
{
    uint32 result = logalloc_expand_datablock(ptr, size);
    if (result == LOGALLOC_OK)
    {
        return ptr;
    }
    void* temp = logalloc_allocate_memory(size, align_bytes);
    /* we need to copy the old data to the new block with OLD size, not new 
     * logalloc_get_datablock_size returns size in WORDS, but memcpy expects BYTES */
    memcpy(temp, ptr, logalloc_get_datablock_size(ptr) * sizeof(uint32));
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
    char filename[256];
    snprintf(filename, sizeof(filename), "logalloc_dump_%u.bin", dump_count++);
    FILE* outfile = fopen(filename, "wb");

    m_assert(outfile != NULL, "failed to open logalloc dump file for writing");

    /* write the entire logalloc_pool buffer to file */
    size_t written = fwrite(logalloc_pool, sizeof(uint32), MAX_POOL_SIZE / sizeof(uint32), outfile);

    m_assert(written == (MAX_POOL_SIZE / sizeof(uint32)), "failed to write to %s", filename);

    fclose(outfile);

    printf("logalloc pool dumped to %s\n", filename);
}


