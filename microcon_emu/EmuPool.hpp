#pragma once
#include "Proxy.hpp"
#include <string.h>

/* to be - generated macros & variables */
#define MAX_POOL_SIZE 0x100000	/* 0x100000 1MB */
#define USE_EMUPOOL /* uncomment to enable EMUPOOL */

// #define RELATIVE_INDEXING /* if defined, we will use relative indexing instead of absolute indexing, which will save space for prev and next index, but will limit the oneshot allocation from 4GB to 16MB */

#ifdef RELATIVE_INDEXING
typedef struct {
    uint32 firstword;   /* [31:24] prev1 | [23:16] next0 | [15:8] prev0 | [7:0] magicnum0 */
    uint32 secondword;  /* [31:24] next2 | [23:16] next1 | [15:8] prev2 | [7:0] magicnum1 */
} logalloc_block_header;    /* 64bit */

#define MAGIC_NUMBER 0xAA
#define MAGIC_NUMBER_FREE 0xCC  /* need at least two bits different for validity check(OR) */


#else
/* type definitions */
typedef struct {
    uint32 magic;
    uint32 prev;
    uint32 next;
} logalloc_block_header;
#define MAGIC_NUMBER 0xAAAAAAAA
#define MAGIC_NUMBER_FREE 0xCCCCCCCC
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

#ifdef RELATIVE_INDEXING
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

/* (...){...} <- this is a compound literal btw; valid C99 */
/* decode macro */
#define RELADR_HEAD_DECODE(index) \
    (logalloc_block_header){ \
        .firstword = (~(index)) ^ (CONV_IDX_TO_ADDR(index)->firstword), \
        .secondword = (index) ^ (CONV_IDX_TO_ADDR(index)->secondword) \
    }

/* encode macro */
#define RELADR_HEAD_ENCODE(index, firstword, secondword) \
    (logalloc_block_header){ \
        .firstword = (~(index)) ^ (firstword), \
        .secondword = (index) ^ (secondword) \
    }

/* update macro for header update; internally uses encode macro */
#define RELADR_HEAD_UPDATE(index, previndex, nextindex) do { \
    uint32 firstword = MAGIC_NUMBER | \
                        (((previndex) >> 16) & 0xFF) << 8 | \
                        (((nextindex) >> 16) & 0xFF) << 16 | \
                        (((previndex) >> 8) & 0xFF) << 24; \
    uint32 secondword = MAGIC_NUMBER | \
                         ((previndex) & 0xFF) << 8 | \
                         (((nextindex) >> 8) & 0xFF) << 16 | \
                         ((nextindex) & 0xFF) << 24; \
    *CONV_IDX_TO_ADDR(index) = RELADR_HEAD_ENCODE(index, firstword, secondword); \
} while(0)

/* get next and prev index macros; internally uses decode macro */
#define RELADR_NEXT_IDX(index) \
    (((RELADR_HEAD_DECODE(index).firstword >> 16) & 0xFF) << 16 | \
     ((RELADR_HEAD_DECODE(index).secondword >> 16) & 0xFF) << 8 | \
     ((RELADR_HEAD_DECODE(index).secondword >> 24) & 0xFF))

#define RELADR_PREV_IDX(index) \
    (((RELADR_HEAD_DECODE(index).firstword >> 8) & 0xFF) << 16 | \
     ((RELADR_HEAD_DECODE(index).firstword >> 24) & 0xFF) << 8 | \
     ((RELADR_HEAD_DECODE(index).secondword >> 8) & 0xFF))

/* get magic number macro; internally uses decode macro */
/* if valid, this should only give value of 0xAA or 0xCC */
#define RELADR_MAGIC_NUMBER(index) \
    ((RELADR_HEAD_DECODE(index).firstword & 0xFF) | \
     (RELADR_HEAD_DECODE(index).secondword & 0xFF))
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



