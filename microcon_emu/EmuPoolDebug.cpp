#include "EmuPoolDebug.hpp"
#include "EmuPool.hpp"
#include <ctime>

#ifdef EMUPOOL_DEBUG_LOGS

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

// Public debug functions

void debug_init() {
    write_history_count = 0;
    history_enabled = true;
    allocated_block_count = 0;
}

void debug_log_write(uint32 index, uint32 old_value, uint32 new_value, const char* operation) {
    log_write(index, old_value, new_value, operation);
}

void debug_register_block(uint32 start_index, uint32 size_in_bytes) {
    if (allocated_block_count >= MAX_ALLOCATED_BLOCKS) {
        m_assert(0, "too many allocated blocks for tracking");
    }
    
    allocated_blocks[allocated_block_count].start_index = start_index;
    allocated_blocks[allocated_block_count].end_index = start_index + (size_in_bytes / 4) - 1;
    allocated_blocks[allocated_block_count].active = true;
    allocated_block_count++;
}

void debug_unregister_block(uint32 start_index) {
    for (uint32 i = 0; i < allocated_block_count; i++) {
        if (allocated_blocks[i].active && allocated_blocks[i].start_index == start_index) {
            allocated_blocks[i].active = false;
            return;
        }
    }
}

void debug_dump_write_history(uint32 corrupt_index) {
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

uint32 debug_get_block_end_index(uint32 start_index) {
    for (uint32 i = 0; i < allocated_block_count; i++) {
        if (allocated_blocks[i].active && allocated_blocks[i].start_index == start_index) {
            return allocated_blocks[i].end_index;
        }
    }
    return 0;
}

bool debug_has_active_block(uint32 start_index) {
    for (uint32 i = 0; i < allocated_block_count; i++) {
        if (allocated_blocks[i].active && allocated_blocks[i].start_index == start_index) {
            return true;
        }
    }
    return false;
}

#endif
