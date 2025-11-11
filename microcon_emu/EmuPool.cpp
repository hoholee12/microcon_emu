#include "EmuPool.hpp"
#include "EmuPoolDebug.hpp"
#include <vector>
#include <ctime>

/* simple memory allocator for emulation structures
 * structure:
 * 1MB buffer
 * 
 * - header -
 * first 4byte: magic number for corruption detection (0xAAAAAAAA)
 * second 4byte: previous block's starting address + 3 (starting address without the header)
 * third 4byte: next block's starting address
 * 
 * if a middle block is freed, previous block's next pointer is set to the next of the freed block
 * freed block's next pointer is set to nullptr
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
 * 2. Check next: if next_block.magic == 0, the next block is already freed
 * 3. When coalescing, update forward links to skip over all freed blocks
 * 4. Example: A -> [freed B] -> [to-free C] -> D becomes A -> D
 * 5. This creates larger contiguous free spaces for bigger allocations
 */

uint32 microcon_emupool[EMUPOOL_BUFFER_SIZE];

void* EmuPool_allocate_clear_memory(uint32 size)
{
    uint32* ptr = NULL;
    ptr = (uint32*)EmuPool_allocate_memory(size);
    memset(ptr, 0, size);

    return ptr;
}

void* EmuPool_allocate_memory(uint32 size) {
    uint32 index = 0;
    uint32 blocksize = size / 4 + 3;	// size in 4byte + 3 for header

    uint32 curr_header_prevblock_startaddr = 0;
    uint32 curr_header_nextblock_startaddr = 0;
    uint32 prev_header_prevblock_startaddr = 0;
    uint32 prev_header_nextblock_startaddr = 0;

    void* result = NULL;
    uint32 search_iteration = 0;

    // very first allocation
    if (microcon_emupool[0] == 0){
        microcon_emupool[0] = 0xAAAAAAAA;
        microcon_emupool[1] = 0;
        microcon_emupool[2] = blocksize;
        microcon_emupool[blocksize] = 0xAAAAAAAA;
        microcon_emupool[blocksize + 1] = 0x3;
#ifdef MORE_DEBUG_LOGS
        debug_log_write(0, 0, 0xAAAAAAAA, "first_allocation:magic");
        debug_log_write(1, 0, 0, "first_allocation:prev");
        debug_log_write(2, 0, blocksize, "first_allocation:next");
        debug_log_write(blocksize, 0, 0xAAAAAAAA, "first_allocation:next_magic");
        debug_log_write(blocksize + 1, 0, 0x3, "first_allocation:next_prev");
#endif

        result = &microcon_emupool[3];
#ifdef MORE_DEBUG_LOGS
        debug_register_block(3, size);
#endif
        
#ifdef MORE_DEBUG_LOGS
        printf("[EmuPool] Allocated %u bytes at index %u (0x%X), range: [%u - %u] (0x%X - 0x%X)\n",
               size, 3, 3, 3, 3 + (size / 4) - 1, 3, 3 + (size / 4) - 1);
        printf("  Search: First allocation (no search needed)\n");
#endif
        
        return result;
    }

    // we have at least one block here
#ifdef MORE_DEBUG_LOGS
    printf("[EmuPool] Searching for %u bytes (%u uint32s)...\n", size, size / 4);
#endif
    while(1){
        if (microcon_emupool[index] != 0xAAAAAAAA){
            // memory corruption detected
#ifdef MORE_DEBUG_LOGS
            debug_dump_write_history(index);
#endif
            m_assert(0, "memory corruption detected in emu pool");
        }

        if (index + blocksize + 3/* next (possible)block's header */ >= EMUPOOL_BUFFER_SIZE) {
            // out of memory
            m_assert(0, "out of memory in emu pool");
        }

        /* current block's header */
        curr_header_prevblock_startaddr = microcon_emupool[index + 1];
        curr_header_nextblock_startaddr = microcon_emupool[index + 2];

#ifdef MORE_DEBUG_LOGS
        printf("  [%u] Checking index %u (0x%X): prev=%u, next=%u\n", 
               search_iteration, index, index, 
               curr_header_prevblock_startaddr, curr_header_nextblock_startaddr);
#endif

        /* one iteration */
        /* check if we are at the position of the last + 1 block */
        if (curr_header_prevblock_startaddr != 0x0 && curr_header_nextblock_startaddr == 0x0){
            /* start adding new block */
            microcon_emupool[index + 2] = index + blocksize;
            /* pre-init for next block */
            microcon_emupool[index + blocksize] = 0xAAAAAAAA;
            microcon_emupool[index + blocksize + 1] = index + 3;
#ifdef MORE_DEBUG_LOGS
            printf("  -> Found end of list, allocating new block at end\n");
            debug_log_write(index + 2, 0, index + blocksize, "allocate_last:next");
            debug_log_write(index + blocksize, 0, 0xAAAAAAAA, "allocate_last:new_magic");
            debug_log_write(index + blocksize + 1, 0, index + 3, "allocate_last:new_prev");
#endif
            
            result = &microcon_emupool[index + 3];
#ifdef MORE_DEBUG_LOGS
            debug_register_block(index + 3, size);
#endif
            
#ifdef MORE_DEBUG_LOGS
            printf("[EmuPool] Allocated %u bytes at index %u (0x%X), range: [%u - %u] (0x%X - 0x%X)\n",
                   size, index + 3, index + 3, 
                   index + 3, index + 3 + (size / 4) - 1,
                   index + 3, index + 3 + (size / 4) - 1);
            printf("  Search: %u iterations to find end of list\n", search_iteration + 1);
#endif
            
            return result;
        }
        /* check if there is a free block between previous block and current block */
        else if (curr_header_prevblock_startaddr != 0x0 && curr_header_nextblock_startaddr != 0x0){
            /* Gap detection: if current block's prev pointer doesn't point to where we expect,
             * there's a freed block in between. The freed block's address should be what 
             * the current block is pointing to as its previous block. */
            uint32 expected_prev_addr = prev_header_nextblock_startaddr + 3; // where previous block's data starts
            
            if (curr_header_prevblock_startaddr != expected_prev_addr) {
                /* There's a gap! The freed block starts at curr_header_prevblock_startaddr - 3 (header) */
                uint32 freed_block_start = curr_header_prevblock_startaddr - 3;
                uint32 gap = index - freed_block_start; /* gap size from freed block start to current block start */
                
#ifdef MORE_DEBUG_LOGS
                printf("  -> Detected gap of %u uint32s (freed block at %u, current at %u)\n", gap, freed_block_start, index);
#endif
                if (gap >= blocksize) {
                /* we have enough space to insert a new block here */
                    microcon_emupool[freed_block_start] = 0xAAAAAAAA;
                    microcon_emupool[freed_block_start + 1] = prev_header_nextblock_startaddr + 3;
                    microcon_emupool[freed_block_start + 2] = freed_block_start + blocksize;
                    /* if there's remaining space after our allocation, set up next block */
                    if (gap > blocksize) {
                        microcon_emupool[freed_block_start + blocksize] = 0xAAAAAAAA;
                        microcon_emupool[freed_block_start + blocksize + 1] = freed_block_start + 3;
                        microcon_emupool[freed_block_start + blocksize + 2] = index;
                    }
#ifdef MORE_DEBUG_LOGS
                    printf("  -> Gap is large enough, reusing freed space at %u\n", freed_block_start);
                    debug_log_write(freed_block_start, 0, 0xAAAAAAAA, "allocate_gap:restore_magic");
                    debug_log_write(freed_block_start + 1, 0, prev_header_nextblock_startaddr + 3, "allocate_gap:restore_prev");
                    debug_log_write(freed_block_start + 2, 0, freed_block_start + blocksize, "allocate_gap:restore_next");
                    if (gap > blocksize) {
                        debug_log_write(freed_block_start + blocksize, 0, 0xAAAAAAAA, "allocate_gap:split_magic");
                        debug_log_write(freed_block_start + blocksize + 1, 0, freed_block_start + 3, "allocate_gap:split_prev");
                        debug_log_write(freed_block_start + blocksize + 2, 0, index, "allocate_gap:split_next");
                    }
#endif
                    
                    result = &microcon_emupool[freed_block_start + 3];
#ifdef MORE_DEBUG_LOGS
                    debug_register_block(freed_block_start + 3, size);
                    
                    printf("[EmuPool] Allocated %u bytes at index %u (0x%X), range: [%u - %u] (0x%X - 0x%X) [GAP REUSE]\n",
                           size, freed_block_start + 3, freed_block_start + 3,
                           freed_block_start + 3, freed_block_start + 3 + (size / 4) - 1,
                           freed_block_start + 3, freed_block_start + 3 + (size / 4) - 1);
                    printf("  Search: %u iterations to find suitable gap\n", search_iteration + 1);
#endif
                    
                    return result;
                } else {
                    /* Gap too small, keep looking */
#ifdef MORE_DEBUG_LOGS
                    printf("  -> Gap too small (need %u), continuing search\n", blocksize);
#endif
                }
            }
        }

        /* update index to the next block */
        index = curr_header_nextblock_startaddr;

        /* update previous entry info */
        prev_header_prevblock_startaddr = curr_header_prevblock_startaddr;
        prev_header_nextblock_startaddr = curr_header_nextblock_startaddr;

        search_iteration++;
    }
}

void EmuPool_Init(){
#ifdef MORE_DEBUG_LOGS
    debug_init();
#endif
    EmuPool_allocate_memory(4); // head entry. it should not ever be used or freed during lifetime
}

void EmuPool_free_memory(void* ptr){
    /* pointer is 64 bit but index is much less than that so this is fine */
    uint32 baseindex = (uint32*)ptr - (uint32*)microcon_emupool;
    if (microcon_emupool[baseindex - 3] != 0xAAAAAAAA){
        // memory corruption detected
#ifdef MORE_DEBUG_LOGS
        debug_dump_write_history(baseindex - 3);
#endif
        m_assert(0, "memory corruption detected in emu pool, or you are passing an invalid pointer");
    }
    uint32 prevblock_startaddr = microcon_emupool[baseindex - 2];
    uint32 nextblock_startaddr = microcon_emupool[baseindex - 1];

    if(baseindex == 3){
        /* due to the nature of this structure, removing the first block means death. */
        m_assert(0, "cannot free the first block in emu pool");
    }

#ifdef MORE_DEBUG_LOGS
    // Find the block to get its range before unregistering
    uint32 end_index = debug_get_block_end_index(baseindex);
    
    if (end_index != 0) {
        uint32 size_in_bytes = (end_index - baseindex + 1) * 4;
        
        printf("[EmuPool] Freed %u bytes at index %u (0x%X), range: [%u - %u] (0x%X - 0x%X)\n",
               size_in_bytes, baseindex, baseindex,
               baseindex, end_index,
               baseindex, end_index);
    }

    // Unregister this block from tracking
    debug_unregister_block(baseindex);
#endif

    // COALESCING LOGIC: Check for adjacent free blocks and merge them
    uint32 final_prevblock_startaddr = prevblock_startaddr;
    uint32 final_nextblock_startaddr = nextblock_startaddr;
    
    // Check if previous block is free by examining if there's a gap before this block
    if (prevblock_startaddr != 0) {
        uint32 prev_block_header = prevblock_startaddr - 3;
        uint32 prev_block_next = microcon_emupool[prev_block_header + 2];
        uint32 expected_next = baseindex - 3; // our block's header address
        
        if (prev_block_next != expected_next) {
            // There's a gap before us! Find the start of the free region
            uint32 scan_addr = prev_block_next;
            
            // Scan backwards through consecutive free blocks to find the start of free region
            while (scan_addr != 0 && microcon_emupool[scan_addr] == 0) {
                uint32 scan_prev = microcon_emupool[scan_addr + 1];
                if (scan_prev == 0) break; // reached the beginning
                
                uint32 scan_prev_header = scan_prev - 3;
                uint32 scan_prev_next = microcon_emupool[scan_prev_header + 2];
                
                if (scan_prev_next == scan_addr) {
                    // Previous block points directly to this scan position - no gap before it
                    break;
                } else {
                    // Continue scanning backward
                    scan_addr = scan_prev_next;
                }
            }
            
            if (scan_addr != 0 && microcon_emupool[scan_addr] == 0) {
#ifdef MORE_DEBUG_LOGS
                printf("  -> Coalescing with previous free region starting at %u\n", scan_addr);
#endif
                // Found the start of free region, use its previous block
                final_prevblock_startaddr = microcon_emupool[scan_addr + 1];
            }
        }
    }
    
    // Check if next block is free and scan forward through consecutive free blocks
    if (nextblock_startaddr != 0) {
        uint32 scan_addr = nextblock_startaddr;
        
        // Scan forward through consecutive free blocks
        while (scan_addr != 0 && microcon_emupool[scan_addr] == 0) {
            // This block is free, check what it points to
            uint32 scan_next = microcon_emupool[scan_addr + 2];
            if (scan_next == 0) {
                // This free block was the last block
                final_nextblock_startaddr = 0;
                break;
            } else if (microcon_emupool[scan_next] == 0) {
                // Next block is also free, continue scanning
                scan_addr = scan_next;
            } else {
                // Next block is allocated, stop here
                final_nextblock_startaddr = scan_next;
                break;
            }
        }
        
        if (final_nextblock_startaddr != nextblock_startaddr) {
#ifdef MORE_DEBUG_LOGS
            printf("  -> Coalescing with next free region ending at %u\n", scan_addr);
#endif
        }
    }

#ifdef MORE_DEBUG_LOGS
    if (final_prevblock_startaddr != prevblock_startaddr || final_nextblock_startaddr != nextblock_startaddr) {
        printf("  -> Block coalescing: prev %u->%u, next %u->%u\n", 
               prevblock_startaddr, final_prevblock_startaddr,
               nextblock_startaddr, final_nextblock_startaddr);
    }
#endif

    // Update the forward link: previous block now points to final next block
    if (final_prevblock_startaddr != 0) {
        uint32 old_val = microcon_emupool[final_prevblock_startaddr - 1];
        microcon_emupool[final_prevblock_startaddr - 1] = final_nextblock_startaddr;
#ifdef MORE_DEBUG_LOGS
        debug_log_write(final_prevblock_startaddr - 1, old_val, final_nextblock_startaddr, "free:coalesce_update_prev_next");
#endif
    }

    // NOTE: We intentionally do NOT update the next block's previous pointer
    // This leaves the next block pointing to the now-freed block, which allows
    // us to detect gaps during allocation by checking for this inconsistency

    /* clear the header */
    uint32 old_magic = microcon_emupool[baseindex - 3];
    uint32 old_prev = microcon_emupool[baseindex - 2];
    uint32 old_next = microcon_emupool[baseindex - 1];
    microcon_emupool[baseindex - 3] = 0;
    microcon_emupool[baseindex - 2] = 0;
    microcon_emupool[baseindex - 1] = 0;
#ifdef MORE_DEBUG_LOGS
    debug_log_write(baseindex - 3, old_magic, 0, "free:clear_magic");
    debug_log_write(baseindex - 2, old_prev, 0, "free:clear_prev");
    debug_log_write(baseindex - 1, old_next, 0, "free:clear_next");
#endif
    /* we do not clear the data area. it is not needed */
}
