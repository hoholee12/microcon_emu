#pragma once
#include "Proxy.hpp"

//#define MORE_DEBUG_LOGS // Uncomment to enable detailed debug logs
#define EMUPOOL_BUFFER_SIZE 0x1000000	// 1MB buffer
// one bit per 1KB block
extern uint32 microcon_emupool[EMUPOOL_BUFFER_SIZE];
extern void* EmuPool_allocate_clear_memory(uint32 size);
extern void* EmuPool_allocate_memory(uint32 size);
extern void EmuPool_free_memory(void* ptr);
extern void EmuPool_Init();

// #define USE_EMUPOOL // uncomment to enable EMUPOOL
#if defined(USE_EMUPOOL)
#define emalloc(x) (uint32*)EmuPool_allocate_memory(x)
#define ecalloc(x, y) (uint32*)EmuPool_allocate_clear_memory(x * y)
#define efree(x) EmuPool_free_memory(x)
#else
#define emalloc(x) (uint32*)malloc(x)
#define ecalloc(x, y) (uint32*)calloc(x, y)
#define efree(x) free(x)
#endif