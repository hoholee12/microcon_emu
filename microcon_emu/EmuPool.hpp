#pragma once
#include "Proxy.hpp"

/* to be - generated macros & variables */
#define MAX_POOL_SIZE 0x1000000	// 1MB pool


// one bit per 1KB block
extern uint32 logalloc_pool[MAX_POOL_SIZE];
extern void* logalloc_allocate_clear_memory(uint32 size);
extern void* logalloc_allocate_memory(uint32 size);
extern void logalloc_free_memory(void* ptr);
extern void logalloc_init();

#define USE_EMUPOOL // uncomment to enable EMUPOOL
#if defined(USE_EMUPOOL)
#define emalloc(x) (uint32*)logalloc_allocate_memory(x)
#define ecalloc(x, y) (uint32*)logalloc_allocate_clear_memory(x * y)
#define efree(x) logalloc_free_memory(x)
#else
#define emalloc(x) (uint32*)malloc(x)
#define ecalloc(x, y) (uint32*)calloc(x, y)
#define efree(x) free(x)
#endif