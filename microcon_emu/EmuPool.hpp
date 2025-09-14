#pragma once
#include "Proxy.hpp"


#define EMUPOOL_BUFFER_SIZE 0x100000	// 1MB buffer
// one bit per 1KB block
extern uint32 microcon_emupool[EMUPOOL_BUFFER_SIZE];
extern void* EmuPool_allocate_memory(uint32 size);
extern void EmuPool_free_memory(void* ptr);
extern void EmuPool_Init();

#define emalloc(x) (uint32*)EmuPool_allocate_memory(x)
#define efree(x) EmuPool_free_memory(x)