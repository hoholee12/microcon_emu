#pragma once
#include "Proxy.hpp"
#include <string.h>

/* to be - generated macros & variables */
#define MAX_POOL_SIZE 0x100000	/* 0x100000 1MB */
#define MAGIC_NUMBER 0xAAAAAAAA
#define MAGIC_NUMBER_FREE 0xBBBBBBBB
#define INDEX_TYPE uint32 /* 16 bits for index, 64KB */

#define RELATIVE_INDEXING /* if defined, we will use relative indexing instead of absolute indexing, which will save space for prev and next index, but will limit the block size to 64KB */
#ifdef RELATIVE_INDEXING
#define INDEX_TYPE uint16
#define UINT16_MAX 0xFFFF
#endif

/* type definitions */
typedef struct {
    uint32 magic;
    /* future TODO: relative addressing with 2 byte sized indexing
     * this will limit blocksize to that type, and will need to have checkpoints throughout the pool, 
     * but itd be worth the tradeoff(shaves off 33% per header) if you are allocating tiny blocks. */
    INDEX_TYPE prev;
    INDEX_TYPE next;
} logalloc_block_header;

/* one bit per 1KB block */
extern uint32 logalloc_pool[];
extern void* logalloc_allocate_clear_memory(uint32 size);
extern void* logalloc_allocate_memory(uint32 size);
extern void* logalloc_realloc_memory(void* ptr, uint32 size);
extern void logalloc_free_memory(void* ptr);
extern void logalloc_init();

/* extra functions for performance */
#ifdef RELATIVE_INDEXING
extern void logalloc_relidxinit();
#endif

/* macro for address conversion */
#define CONV_IDX_TO_ADDR(index) ((logalloc_block_header*)&logalloc_pool[index])
#define CONV_ADDR_TO_BODY(addr) ((void*)((logalloc_block_header*)addr + 1))
#define CONV_IDX_TO_BODY(index) CONV_ADDR_TO_BODY(CONV_IDX_TO_ADDR(index))

#define USE_EMUPOOL // uncomment to enable EMUPOOL
#if defined(USE_EMUPOOL)
#define emalloc(size) (uint32*)logalloc_allocate_memory(size)
#define ecalloc(elem, size) (uint32*)logalloc_allocate_clear_memory(elem * size)
#define erealloc(ptr, size) (uint32*)logalloc_realloc_memory(ptr, size)
#define efree(ptr) logalloc_free_memory(ptr)
#else
#define emalloc(size) (uint32*)malloc(size)
#define ecalloc(elem, size) (uint32*)calloc(elem, size)
#define erealloc(ptr, size) (uint32*)realloc(ptr, size)
#define efree(ptr) free(ptr)
#endif

#define ememcpy(dest, src, size) memcpy(dest, src, size)