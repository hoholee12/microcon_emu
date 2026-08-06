#pragma once
#include "Proxy.hpp"
#include <string.h>

/* to be - generated macros & variables */
#define MAX_POOL_SIZE 0x10000000	/* 256MB */
#define USE_EMUPOOL /* uncomment to enable EMUPOOL */

// #define RELATIVE_INDEXING /* if defined, we will use relative indexing instead of absolute indexing, which will save space for prev and next index, but will limit the oneshot allocation from 4GB to 16MB */

#define MEDIAN_SENTINEL_DISTANCE UINT24_MAX /* (for relative indexing) median sentinel distance can be customized. default max is UINT24_MAX. any number larger than this is ignored btw */

#define USE_DEBUG_BLOCK /* if defined, we will allocate one block to store debug information */

#ifdef USE_DEBUG_BLOCK
#define DEBUG_MAGIC_NUMBER 0x13371337
typedef struct {
    uint32 debug_magic; /* 0x13371337 */
    uint32 debug_last_pos;
    uint32 debug_last_pos_perf_penalty;

    /* units are in word */
    uint32 debug_logalloc_pool_cap;
    uint32 debug_logalloc_totalsize; /* total size of the logalloc pool - update only at init */
} logalloc_debug_block;
#endif

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

#define LOGALLOC_OK 0
#define LOGALLOC_ERROR_OUT_OF_MEMORY 1
#define LOGALLOC_ERROR_INVALID_POINTER 2
#define LOGALLOC_ERROR_MEMORY_CORRUPTION 3
#define LOGALLOC_ERROR_ENDSENTINEL_BLOCK 4
#define LOGALLOC_ERROR_UNKNOWN 5


/* one bit per 1KB block */
extern uint32* logalloc_pool;
extern void* logalloc_allocate_clear_memory(uint32 size, uint32 align_bytes);
extern void* logalloc_allocate_memory(uint32 size, uint32 align_bytes);
extern void* logalloc_realloc_memory(void* ptr, uint32 size, uint32 align_bytes);
extern void logalloc_free_memory(void* ptr);
extern void logalloc_init();
extern void logalloc_reposition_last_pos(void* ptr);
extern void logalloc_dump_pool();

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
 * */

/* decode function */
static inline logalloc_block_header RELADR_HEAD_DECODE(uint32 index) {
    logalloc_block_header result;
    result.firstword = (~index) ^ (CONV_IDX_TO_ADDR(index)->firstword);
    result.secondword = index ^ (CONV_IDX_TO_ADDR(index)->secondword);
    return result;
}

/* encode function */
static inline logalloc_block_header RELADR_HEAD_ENCODE(uint32 index, uint32 firstword, uint32 secondword) {
    logalloc_block_header result;
    result.firstword = (~index) ^ firstword;
    result.secondword = index ^ secondword;
    return result;
}

/* update function for header update; internally uses encode function */
static inline void RELADR_HEAD_UPDATE(uint32 index, uint32 prevoffset, uint32 nextoffset) {
    uint32 firstword = MAGIC_NUMBER |
                       (((prevoffset) >> 16) & 0xFF) << 8 |
                       (((nextoffset) >> 16) & 0xFF) << 16 |
                       (((prevoffset) >> 8) & 0xFF) << 24;
    uint32 secondword = MAGIC_NUMBER |
                        ((prevoffset) & 0xFF) << 8 |
                        (((nextoffset) >> 8) & 0xFF) << 16 |
                        ((nextoffset) & 0xFF) << 24;
    *CONV_IDX_TO_ADDR(index) = RELADR_HEAD_ENCODE(index, firstword, secondword);
}

/* update a free block header */
static inline void RELADR_HEAD_UPDATE_FREE(uint32 index, uint32 prevoffset) {
    uint32 firstword = MAGIC_NUMBER_FREE |
                       (((prevoffset) >> 16) & 0xFF) << 8 |
                       (((prevoffset) >> 8) & 0xFF) << 24;
    uint32 secondword = MAGIC_NUMBER_FREE |
                        ((prevoffset) & 0xFF) << 8 |
                        (0 << 16) |         /* nextoffset[15:8] = 0 (free blocks have no defined next) */
                        (0 << 24);          /* nextoffset[23:16] = 0 */
    *CONV_IDX_TO_ADDR(index) = RELADR_HEAD_ENCODE(index, firstword, secondword);
}

static inline uint32 RELADR_NEXT_OFFSET(uint32 index) {
    logalloc_block_header decoded = RELADR_HEAD_DECODE(index);
    uint32 next_offset = (((decoded.firstword >> 16) & 0xFF) << 16 |
            ((decoded.secondword >> 16) & 0xFF) << 8 |
            ((decoded.secondword >> 24) & 0xFF));
    return next_offset;
}

static inline uint32 RELADR_PREV_OFFSET(uint32 index) {
    logalloc_block_header decoded = RELADR_HEAD_DECODE(index);
    uint32 prev_offset = (((decoded.firstword >> 8) & 0xFF) << 16 |
            ((decoded.firstword >> 24) & 0xFF) << 8 |
            ((decoded.secondword >> 8) & 0xFF));
    return prev_offset;
}

/* get next and prev index functions; internally uses decode function */
static inline uint32 RELADR_NEXT_IDX(uint32 index) {
    uint32 next_offset = RELADR_NEXT_OFFSET(index);
    if (next_offset + index >= (MAX_POOL_SIZE / sizeof(uint32))) {
        /* overflow wraparound 
         * for 1MB, 0x0~0x3FFFF is the valid range while 0x40000 and up wraps around */
        return next_offset - ((MAX_POOL_SIZE / sizeof(uint32)) - index);
    }
    else {
        return index + next_offset;
    }
}

static inline uint32 RELADR_PREV_IDX(uint32 index) {
    uint32 prev_offset = RELADR_PREV_OFFSET(index);
    if (prev_offset > index) {
        /* underflow wraparound
         * no wrap on 0x0, only wraps if net negative */
        return (MAX_POOL_SIZE / sizeof(uint32)) - (prev_offset - index);
    }
    else {
        return index - prev_offset;
    }
}

/* get magic number function; internally uses decode function */
/* if valid, this should only give value of 0xAA or 0xCC */
static inline uint32 RELADR_MAGIC_NUMBER(uint32 index) {
    logalloc_block_header decoded = RELADR_HEAD_DECODE(index);
    return ((decoded.firstword & 0xFF) | (decoded.secondword & 0xFF));
}
#endif





#if defined(USE_EMUPOOL)
#define BASE_ALLOC 32
/* TODO: align_bytes set to BASE_ALLOC for now */
#define emalloc(size) (uint32*)logalloc_allocate_memory(size, BASE_ALLOC)
#define ecalloc(elem, size) (uint32*)logalloc_allocate_clear_memory(elem * size, BASE_ALLOC)
#define erealloc(ptr, size) (uint32*)logalloc_realloc_memory(ptr, size, BASE_ALLOC)
#define efree(ptr) logalloc_free_memory(ptr)
#else
#define emalloc(size) (uint32*)malloc(size)
#define ecalloc(elem, size) (uint32*)calloc(elem, size)
#define erealloc(ptr, size) (uint32*)realloc(ptr, size)
#define efree(ptr) free(ptr)
#endif

#define ememcpy(dest, src, size) memcpy(dest, src, size)



