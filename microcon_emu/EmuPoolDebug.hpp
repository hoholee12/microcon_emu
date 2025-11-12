#pragma once
#include "Proxy.hpp"

#ifdef MORE_DEBUG_LOGS

// Forward declaration
extern uint32 microcon_emupool[];

// Debug helper functions
void debug_init();
void debug_log_write(uint32 index, uint32 old_value, uint32 new_value, const char* operation);
void debug_register_block(uint32 start_index, uint32 size_in_bytes);
void debug_unregister_block(uint32 start_index);
void debug_dump_write_history(uint32 corrupt_index);
uint32 debug_get_block_end_index(uint32 start_index);
bool debug_has_active_block(uint32 start_index);

// Debug macros that call the functions
#define DEBUG_INIT() debug_init()
#define DEBUG_LOG_WRITE(index, old_val, new_val, op) debug_log_write(index, old_val, new_val, op)
#define DEBUG_REGISTER_BLOCK(start, size) debug_register_block(start, size)
#define DEBUG_UNREGISTER_BLOCK(start) debug_unregister_block(start)
#define DEBUG_DUMP_HISTORY(index) debug_dump_write_history(index)
#define DEBUG_GET_BLOCK_END(start) debug_get_block_end_index(start)
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)

#else

// No-op macros when debug is disabled
#define DEBUG_INIT() ((void)0)
#define DEBUG_LOG_WRITE(index, old_val, new_val, op) ((void)0)
#define DEBUG_REGISTER_BLOCK(start, size) ((void)0)
#define DEBUG_UNREGISTER_BLOCK(start) ((void)0)
#define DEBUG_DUMP_HISTORY(index) ((void)0)
#define DEBUG_GET_BLOCK_END(start) (0)
#define DEBUG_PRINTF(...) ((void)0)

#endif
