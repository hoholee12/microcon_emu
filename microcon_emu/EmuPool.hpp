#pragma once
#include "Proxy.hpp"

/* to be - generated macros & variables */
#define MAX_POOL_SIZE 0x1000000	/* 64KB pool */
#define MAGIC_NUMBER 0xAAAAAAAA
#define MAGIC_NUMBER_FREE 0xBBBBBBBB
#define INDEX_TYPE uint32 /* 16 bits for index, 64KB */

/* type definitions */
typedef struct {
    uint32 magic;
    INDEX_TYPE prev;
    INDEX_TYPE next;
} logalloc_block_header;

/* one bit per 1KB block */
extern uint32 logalloc_pool[MAX_POOL_SIZE];
extern void* logalloc_allocate_clear_memory(uint32 size);
extern void* logalloc_allocate_memory(uint32 size);
extern void logalloc_free_memory(void* ptr);
extern void logalloc_init();

/* macro for address conversion */
#define CONV_IDX_TO_ADDR(index) ((logalloc_block_header*)&logalloc_pool[index])
#define CONV_ADDR_TO_BODY(addr) ((void*)((logalloc_block_header*)addr + 1))
#define CONV_IDX_TO_BODY(index) CONV_ADDR_TO_BODY(CONV_IDX_TO_ADDR(index))

/* TODO: add logalloc_getsize() to enable erealloc() */

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