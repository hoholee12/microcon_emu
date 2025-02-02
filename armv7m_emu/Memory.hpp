#pragma once

#include "Proxy.hpp"


// basic memory map for armv7m emulation

// memory write supported upto 4 byte writes in little-endian. only 1 byte supported for read.

/* we ignore cache for now
* <basic map>
* 0x0 ~ 0x1FFFFFFF: Code (vector table + ROM)
* 0x20000000 ~ 0x3FFFFFFF: SRAM
* 0x40000000 ~ 0x5FFFFFFF: Peripheral
* 0x60000000 ~ 0xDFFFFFFF: Some sh*t
* 0xE0000000 ~ 0xFFFFFFFF: PPB
* total 5 maps to produce
*/

/*
* Code shall be 128KB (0x0 ~ 0x20000)
* SRAM shall start at 0x20000000 and be 128KB (0x20000000 ~ 0x20020000)
*/

// typedefs / enums

enum Memory_enum_size {u8, u16, u32};
struct Memory_map_elem {
	uint32 base;
	uint32 size;
	uint8* data;	// malloc memory (uint8 for byte access)
	// for quicker access to next map
	struct Memory_map_elem* next;
	//struct Memory_map_elem* prev;
};


// variables

extern Memory_map_elem Memory_var_arr[];
extern uint32 Memory_var_arrlen;

extern uint32 Memory_var_endianness;	// 0: big(0x1234 -> 0:0x12, 1:0x34), 1: little(0x1234, 0:0x34, 1:0x12)

// functions

// memory init map
// size is in bytes (e.g. 128KB -> size: 0x20000)
extern void Memory_addMap(uint32 base, uint32 size);
extern void Memory_init();


// memory read
// size -> 8/16/32 bit
extern void* Memory_read(uint32 addr, Memory_enum_size sizetype);

// memory write
// size -> 8/16/32 bit
extern void Memory_write(uint32 addr, Memory_enum_size sizetype, uint32 data);


// get memory map
extern Memory_map_elem* Memory_getMap(uint32 addr);
