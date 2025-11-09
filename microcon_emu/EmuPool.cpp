#include "EmuPool.hpp"
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
 * when we dealloc block B:
 * block A -> block B -> block C
 * block A -> update to point to block C
 * block C -> do not update.(points to nonexistant block B)
 * when checking for gap, we can use this to calculate gap size, and insert if possible.
 */

uint32 microcon_emupool[EMUPOOL_BUFFER_SIZE];

#ifdef MORE_DEBUG_LOGS
// Allocated block tracking
struct AllocatedBlock {
    uint32 start_index;  // Start of user data (after header)
    uint32 end_index;    // End of user data
    bool active;
};

#define MAX_ALLOCATED_BLOCKS 1000
static AllocatedBlock allocated_blocks[MAX_ALLOCATED_BLOCKS];
static uint32 allocated_block_count = 0;

// Write history tracking
struct WriteHistory {
    uint32 index;
    uint32 old_value;
    uint32 new_value;
    const char* operation;
    time_t timestamp;
};

#define MAX_WRITE_HISTORY 10000
static WriteHistory write_history[MAX_WRITE_HISTORY];
static uint32 write_history_count = 0;
static bool history_enabled = true;
#endif

#ifdef MORE_DEBUG_LOGS
// Helper function to find which block owns an index
static int find_owning_block(uint32 index) {
    for (uint32 i = 0; i < allocated_block_count; i++) {
        if (allocated_blocks[i].active && 
            index >= allocated_blocks[i].start_index && 
            index <= allocated_blocks[i].end_index) {
            return i;
        }
    }
    return -1;
}

// Helper function to register an allocated block
static void register_block(uint32 start_index, uint32 size_in_bytes) {
    if (allocated_block_count >= MAX_ALLOCATED_BLOCKS) {
        m_assert(0, "too many allocated blocks for tracking");
    }
    
    allocated_blocks[allocated_block_count].start_index = start_index;
    allocated_blocks[allocated_block_count].end_index = start_index + (size_in_bytes / 4) - 1;
    allocated_blocks[allocated_block_count].active = true;
    allocated_block_count++;
}

// Helper function to unregister a block
static void unregister_block(uint32 start_index) {
    for (uint32 i = 0; i < allocated_block_count; i++) {
        if (allocated_blocks[i].active && allocated_blocks[i].start_index == start_index) {
            allocated_blocks[i].active = false;
            return;
        }
    }
}

// Helper function to log writes
static void log_write(uint32 index, uint32 old_value, uint32 new_value, const char* operation) {
    if (!history_enabled) return;
    
    uint32 pos = write_history_count % MAX_WRITE_HISTORY;
    write_history[pos].index = index;
    write_history[pos].old_value = old_value;
    write_history[pos].new_value = new_value;
    write_history[pos].operation = operation;
    write_history[pos].timestamp = time(NULL);
    write_history_count++;
}

// Helper function to write to pool with logging
static void write_pool(uint32 index, uint32 value, const char* operation) {
    uint32 old_value = microcon_emupool[index];
    microcon_emupool[index] = value;
    log_write(index, old_value, value, operation);
}

// Function to dump write history for a specific index
static void dump_write_history(uint32 corrupt_index) {
    printf("\n=== Write History for Index %u (0x%X) ===\n", corrupt_index, corrupt_index);
    
    // Check if this index belongs to any allocated block
    int owning_block = find_owning_block(corrupt_index);
    if (owning_block >= 0) {
        printf("*** WARNING: This index is within allocated user block #%d ***\n", owning_block);
        printf("*** Block range: [%u - %u] (0x%X - 0x%X) ***\n",
               allocated_blocks[owning_block].start_index,
               allocated_blocks[owning_block].end_index,
               allocated_blocks[owning_block].start_index,
               allocated_blocks[owning_block].end_index);
        printf("*** Corruption likely caused by user code writing to this allocated memory ***\n");
        printf("*** User should not write beyond their allocated block boundaries ***\n\n");
    }
    
    uint32 start = write_history_count > MAX_WRITE_HISTORY ? 
                   write_history_count - MAX_WRITE_HISTORY : 0;
    uint32 count = write_history_count > MAX_WRITE_HISTORY ? 
                   MAX_WRITE_HISTORY : write_history_count;
    
    bool found_any = false;
    for (uint32 i = 0; i < count; i++) {
        uint32 pos = (start + i) % MAX_WRITE_HISTORY;
        if (write_history[pos].index == corrupt_index) {
            found_any = true;
            char time_buf[26];
            ctime_s(time_buf, sizeof(time_buf), &write_history[pos].timestamp);
            time_buf[24] = '\0'; // Remove newline
            
            printf("[%u] Time: %s, Operation: %s, Old: 0x%08X, New: 0x%08X\n",
                   i,
                   time_buf,
                   write_history[pos].operation,
                   write_history[pos].old_value,
                   write_history[pos].new_value);
        }
    }
    
    if (!found_any) {
        printf("No write history found for this index in tracked operations.\n");
        if (owning_block >= 0) {
            printf("This means the corruption came from direct user writes to allocated memory.\n");
        }
    }
    printf("=== End of Write History ===\n\n");
}
#endif

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
#ifdef MORE_DEBUG_LOGS
        write_pool(0, 0xAAAAAAAA, "first_allocation:magic");
        write_pool(1, 0, "first_allocation:prev");
        write_pool(2, blocksize, "first_allocation:next");
        write_pool(blocksize, 0xAAAAAAAA, "first_allocation:next_magic");
        write_pool(blocksize + 1, 0x3, "first_allocation:next_prev");
#else
        microcon_emupool[0] = 0xAAAAAAAA;
        microcon_emupool[1] = 0;
        microcon_emupool[2] = blocksize;
        microcon_emupool[blocksize] = 0xAAAAAAAA;
        microcon_emupool[blocksize + 1] = 0x3;
#endif

        result = &microcon_emupool[3];
#ifdef MORE_DEBUG_LOGS
        register_block(3, size);
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
            dump_write_history(index);
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
#ifdef MORE_DEBUG_LOGS
            printf("  -> Found end of list, allocating new block at end\n");
            write_pool(index + 2, index + blocksize, "allocate_last:next");
            /* pre-init for next block */
            write_pool(index + blocksize, 0xAAAAAAAA, "allocate_last:new_magic");
            write_pool(index + blocksize + 1, index + 3, "allocate_last:new_prev");
#else
            microcon_emupool[index + 2] = index + blocksize;
            /* pre-init for next block */
            microcon_emupool[index + blocksize] = 0xAAAAAAAA;
            microcon_emupool[index + blocksize + 1] = index + 3;
#endif
            
            result = &microcon_emupool[index + 3];
#ifdef MORE_DEBUG_LOGS
            register_block(index + 3, size);
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
            /* when dealloc, the next block's prevaddr is not updated. we use that to figure out the size */
            /* we dont keep the block size itself so this is the only way to figure out the size of the missing block */
            uint32 gap = curr_header_nextblock_startaddr /* next next block */ - microcon_emupool[curr_header_nextblock_startaddr + 1] /* end of valid block + 1 */;
#ifdef MORE_DEBUG_LOGS
            printf("  -> Detected gap of %u uint32s between blocks\n", gap);
#endif
            if (gap >= blocksize){
                /* we have enough space to insert a new block here */
#ifdef MORE_DEBUG_LOGS
                printf("  -> Gap is large enough, reusing freed space\n");
                write_pool(index + 2, index + blocksize, "allocate_gap:next");
                /* pre-init for next block */
                write_pool(index + blocksize, 0xAAAAAAAA, "allocate_gap:new_magic");
                write_pool(index + blocksize + 1, prev_header_nextblock_startaddr + 3, "allocate_gap:new_prev");
                /* update deleted entry */
                write_pool(curr_header_nextblock_startaddr + 1, 
                          microcon_emupool[curr_header_nextblock_startaddr + 1] + blocksize, 
                          "allocate_gap:update_next_prev");
#else
                microcon_emupool[index + 2] = index + blocksize;
                /* pre-init for next block */
                microcon_emupool[index + blocksize] = 0xAAAAAAAA;
                microcon_emupool[index + blocksize + 1] = prev_header_nextblock_startaddr + 3;
                /* update deleted entry */
                microcon_emupool[curr_header_nextblock_startaddr + 1] = 
                    microcon_emupool[curr_header_nextblock_startaddr + 1] + blocksize;
#endif
                
                result = &microcon_emupool[index + 3];
#ifdef MORE_DEBUG_LOGS
                register_block(index + 3, size);
#endif
                
#ifdef MORE_DEBUG_LOGS
                printf("[EmuPool] Allocated %u bytes at index %u (0x%X), range: [%u - %u] (0x%X - 0x%X) [GAP REUSE]\n",
                       size, index + 3, index + 3,
                       index + 3, index + 3 + (size / 4) - 1,
                       index + 3, index + 3 + (size / 4) - 1);
                printf("  Search: %u iterations to find suitable gap\n", search_iteration + 1);
#endif
                
                return result;
            }
            /* else, keep looking */
#ifdef MORE_DEBUG_LOGS
            printf("  -> Gap too small (need %u), continuing search\n", blocksize);
#endif
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
    write_history_count = 0;
    history_enabled = true;
    allocated_block_count = 0;
#endif
    EmuPool_allocate_memory(4); // head entry. it should not ever be used or freed during lifetime
}

void EmuPool_free_memory(void* ptr){
    /* pointer is 64 bit but index is much less than that so this is fine */
    uint32 baseindex = (uint32*)ptr - (uint32*)microcon_emupool;
    if (microcon_emupool[baseindex - 3] != 0xAAAAAAAA){
        // memory corruption detected
#ifdef MORE_DEBUG_LOGS
        dump_write_history(baseindex - 3);
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
    int block_idx = -1;
    uint32 end_index = 0;
    for (uint32 i = 0; i < allocated_block_count; i++) {
        if (allocated_blocks[i].active && allocated_blocks[i].start_index == baseindex) {
            block_idx = i;
            end_index = allocated_blocks[i].end_index;
            break;
        }
    }
    
    uint32 size_in_bytes = (end_index - baseindex + 1) * 4;
    
    printf("[EmuPool] Freed %u bytes at index %u (0x%X), range: [%u - %u] (0x%X - 0x%X)\n",
           size_in_bytes, baseindex, baseindex,
           baseindex, end_index,
           baseindex, end_index);

    // Unregister this block from tracking
    unregister_block(baseindex);
#endif

    if (nextblock_startaddr != 0){
        /* not the last block */
#ifdef MORE_DEBUG_LOGS
        write_pool(nextblock_startaddr + 1, prevblock_startaddr, "free:update_next_prev");
#else
        microcon_emupool[nextblock_startaddr + 1] = prevblock_startaddr;
#endif
    }

#ifdef MORE_DEBUG_LOGS
    write_pool(prevblock_startaddr - 1, nextblock_startaddr, "free:update_prev_next");
    /* clear the header */
    write_pool(baseindex - 3, 0, "free:clear_magic");
    write_pool(baseindex - 2, 0, "free:clear_prev");
    write_pool(baseindex - 1, 0, "free:clear_next");
#else
    microcon_emupool[prevblock_startaddr - 1] = nextblock_startaddr;
    /* clear the header */
    microcon_emupool[baseindex - 3] = 0;
    microcon_emupool[baseindex - 2] = 0;
    microcon_emupool[baseindex - 1] = 0;
#endif
    /* we do not clear the data area. it is not needed */
}
