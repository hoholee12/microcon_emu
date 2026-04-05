#pragma once
#include "Proxy.hpp"
#include <string.h>

/* to be - generated macros & variables */
#define MAX_POOL_SIZE 0x100000	/* 0x100000 1MB */
#define USE_EMUPOOL /* uncomment to enable EMUPOOL */

// #define RELATIVE_INDEXING /* if defined, we will use relative indexing instead of absolute indexing, which will save space for prev and next index, but will limit the oneshot allocation from 4GB to 16MB */

#ifdef RELATIVE_INDEXING
typedef struct {
    uint8 magicnum0; /* always expects AA or BB (free) */
    uint8 prev0;
    uint8 next0;
    uint8 prev1;
    uint8 magicnum1; /* always expects AA or BB (free) */
    uint8 prev2;
    uint8 next1;
    uint8 next2;
} logalloc_block_header;    /* 64bit */

#define MAGIC_NUMBER 0xAA
#define MAGIC_NUMBER_FREE 0xBB


#else
/* type definitions */
typedef struct {
    uint32 magic;
    uint32 prev;
    uint32 next;
} logalloc_block_header;
#define MAGIC_NUMBER 0xAAAAAAAA
#define MAGIC_NUMBER_FREE 0xBBBBBBBB
#endif


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

#ifdef RELATIVE_ADDRESSING
#define RELADR_HEAD_DECODE_0(index) (((~index) ^ (CONV_IDX_TO_ADDR(index)->magicnum0 + )))
#define RELADR_HEAD_DECODE_1(index) 
#define RELADR_HEAD_ENCODE_0(index)
#define RELADR_HEAD_ENCODE_1(index)
#define RELADR_NEXT_IDX(index) \
do {
    CONV_IDX_TO_ADDR(index);


} while(0);
#define RELADR_PREV_IDX
#endif





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





/* make block headers as reliable as possible
 * --- we will use 2 words (8bytes) for best alignment.
 * structure:
 * 
 * word #1:
 * - real index position(inverted) as secondary encoding
 * - byte 1: AA as encoded magic number
 * - byte 2-3: 1st byte for each prev / next index
 * - byte 4: 2nd byte of prev index
 * 
 * word #2:
 * - word #1's index position as secondary encoding
 * - byte 1: AA as encoded magic number
 * - byte 2: 3rd byte of prev index
 * - byte 3-4: 2nd-3rd byte of next index
 * 
 * index position for encoding:
 * - 4 byte inverted index + 4 byte index = 8 byte
 * 
 * 
 * demo:
 * AA 00 00 00 AA 0F 00 0F (prev is 0F away, next is 0F away)
 * position XOR: (example AB CD EF 01)
 * negation - 54 32 10 FF
 * ^ 543210FFABCDEF01 -> (~index) + index
 * FE 32 10 FF 01 C2 EF 0E
 * 
 * i call this ~position-dependent XOR encoding with bidirectional mixing~
 * 
 * */