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
    uint32 prev_block_header_index = 0; // Track the previous block's header address

    void* result = NULL;
    uint32 search_iteration = 0;

    // very first allocation
    if (microcon_emupool[0] == 0){
        microcon_emupool[0] = 0xAAAAAAAA;
        microcon_emupool[1] = 0;
        microcon_emupool[2] = blocksize;
        microcon_emupool[blocksize] = 0xAAAAAAAA;
        microcon_emupool[blocksize + 1] = 0x3;
        microcon_emupool[blocksize + 2] = 0; /* end marker has next = 0 */

        result = &microcon_emupool[3];
        return result;
    }

    // we have at least one block here
    while(1){
        if (microcon_emupool[index] != 0xAAAAAAAA){
            // memory corruption detected
            m_assert(0, "memory corruption detected in emu pool");
        }

        if (index + blocksize + 3/* next (possible)block's header */ >= EMUPOOL_BUFFER_SIZE) {
            // out of memory
            m_assert(0, "out of memory in emu pool");
        }

        /* current block's header */
        curr_header_prevblock_startaddr = microcon_emupool[index + 1];
        curr_header_nextblock_startaddr = microcon_emupool[index + 2];

        /* one iteration */
        /* check if we are at the position of the last + 1 block */
        if (curr_header_prevblock_startaddr != 0x0 && curr_header_nextblock_startaddr == 0x0){
            /* We found the end marker. Convert it to a real block and create new end marker. */
            /* Current block at 'index' will become our new allocated block */
            microcon_emupool[index + 2] = index + blocksize; /* update next pointer to new end marker */
            /* Create new end marker */
            microcon_emupool[index + blocksize] = 0xAAAAAAAA;
            microcon_emupool[index + blocksize + 1] = index + 3; /* prev points to our new block's data */
            microcon_emupool[index + blocksize + 2] = 0; /* next is 0 (end marker) */
            result = &microcon_emupool[index + 3];
            return result;
        }
        /* check if there is a free block between previous block and current block */
        else if (curr_header_prevblock_startaddr != 0x0 && curr_header_nextblock_startaddr != 0x0){
            /* Gap detection: if current block's prev pointer doesn't point to where we expect,
             * there's a freed block in between. The freed block's address should be what 
             * the current block is pointing to as its previous block. */
            
            /* Skip gap detection for the very first block (index 0) since it has no previous block */
            if (search_iteration > 0) {
                uint32 expected_prev_addr = prev_block_header_index + 3; // where previous block's data starts
                
                if (curr_header_prevblock_startaddr != expected_prev_addr) {
                    /* There's a gap! The freed block starts at curr_header_prevblock_startaddr - 3 (header) */
                    uint32 freed_block_start = curr_header_prevblock_startaddr - 3;
                    uint32 gap = index - freed_block_start; /* gap size from freed block start to current block start */
                    
                    if (gap >= blocksize) {
                    /* we have enough space to insert a new block here */
                        microcon_emupool[freed_block_start] = 0xAAAAAAAA;
                        microcon_emupool[freed_block_start + 1] = prev_block_header_index + 3;
                        microcon_emupool[freed_block_start + 2] = freed_block_start + blocksize;
                        
                        /* CRITICAL: Update the previous block's next pointer to point to our new block */
                        microcon_emupool[prev_block_header_index + 2] = freed_block_start;
                        
                        /* if there's remaining space after our allocation, set up next block */
                        if (gap > blocksize) {
                            microcon_emupool[freed_block_start + blocksize] = 0xAAAAAAAA;
                            microcon_emupool[freed_block_start + blocksize + 1] = freed_block_start + 3;
                            microcon_emupool[freed_block_start + blocksize + 2] = index;
                        } else {
                            /* Gap exactly fits - update current block's prev to point to our new block */
                            microcon_emupool[index + 1] = freed_block_start + 3;
                        }
                        
                        result = &microcon_emupool[freed_block_start + 3];                    
                        return result;
                    } else {
                        /* Gap too small, keep looking */
                    }
                }
            }
        }

        /* update index to the next block */
        prev_block_header_index = index; // Save current index before moving to next
        index = curr_header_nextblock_startaddr;

        search_iteration++;
    }
}

void EmuPool_Init(){
    EmuPool_allocate_memory(4); // head entry. it should not ever be used or freed during lifetime
}

void EmuPool_free_memory(void* ptr){
    /* pointer is 64 bit but index is much less than that so this is fine */
    uint32 baseindex = (uint32*)ptr - (uint32*)microcon_emupool;
    if (microcon_emupool[baseindex - 3] != 0xAAAAAAAA){
        // memory corruption detected
        m_assert(0, "memory corruption detected in emu pool, or you are passing an invalid pointer");
    }
    uint32 prevblock_startaddr = microcon_emupool[baseindex - 2];
    uint32 nextblock_startaddr = microcon_emupool[baseindex - 1];

    if(baseindex == 3){
        /* due to the nature of this structure, removing the first block means death. */
        m_assert(0, "cannot free the first block in emu pool");
    }

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
            
            // If prev_block_next is 0, it means the previous block was freed and cleared
            // In this case, we need to find the actual allocated block before it
            if (scan_addr == 0) {
                // The previous block is freed. Walk backwards through the chain of freed blocks
                // until we find an allocated block
                uint32 walk_addr = prev_block_header;
                
                while (walk_addr != 0 && microcon_emupool[walk_addr] == 0) {
                    // This block is freed, get its prev pointer
                    uint32 walk_prev_data = microcon_emupool[walk_addr + 1];
                    if (walk_prev_data == 0) {
                        break;
                    }
                    uint32 walk_prev_header = walk_prev_data - 3;
                    
                    // Check if the previous block is allocated
                    if (microcon_emupool[walk_prev_header] == 0xAAAAAAAA) {
                        // Found an allocated block!
                        final_prevblock_startaddr = walk_prev_data;
                        break;
                    }
                    // Previous block is also freed, continue walking
                    walk_addr = walk_prev_header;
                }
            } else {
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
                    // Found the start of free region, use its previous block
                    final_prevblock_startaddr = microcon_emupool[scan_addr + 1];
                }
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
    }

    // Update the forward link: previous block now points to final next block
    if (final_prevblock_startaddr != 0) {
        uint32 old_val = microcon_emupool[final_prevblock_startaddr - 1];
        microcon_emupool[final_prevblock_startaddr - 1] = final_nextblock_startaddr;
    }

    // NOTE: We intentionally do NOT update the next block's previous pointer
    // This leaves the next block pointing to the now-freed block, which allows
    // us to detect gaps during allocation by checking for this inconsistency

    /* Clear the header to mark block as freed
     * IMPORTANT: We only clear magic and next, but KEEP the prev pointer!
     * The prev pointer is needed for coalescing when freeing adjacent blocks.
     * The coalescing logic needs magic==0 to detect freed blocks.
     * The forward chain should never point to freed blocks, so allocation won't see them */
    uint32 old_magic = microcon_emupool[baseindex - 3];
    uint32 old_next = microcon_emupool[baseindex - 1];
    microcon_emupool[baseindex - 3] = 0; // Clear magic
    // microcon_emupool[baseindex - 2] = 0; // DO NOT clear prev - we need it for coalescing!
    microcon_emupool[baseindex - 1] = 0; // Clear next
    /* we do not clear the data area. it is not needed */
}
