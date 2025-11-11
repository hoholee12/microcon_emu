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

#endif
