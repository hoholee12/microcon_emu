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
	uint32 attrib;
	uint32 nd_attrib;
	uint8* data;	// malloc memory (uint8 for byte access)
	// for quicker access to next map
	struct Memory_map_elem* next;
	//struct Memory_map_elem* prev;
};


// variables
#define MEMORY_MAP_MAX_SECTIONS 10
extern Memory_map_elem Memory_var_arr[];
extern uint32 Memory_var_arrlen;

extern uint32 Memory_var_endianness;	// 0: big(0x1234 -> 0:0x12, 1:0x34), 1: little(0x1234, 0:0x34, 1:0x12)


/* allow memory attribute for all
* -> r, w, x, cacheable, ordered
* -> user, supervisor
*/
#define MEMORY_ATTRIB_U_R 0x1
#define MEMORY_ATTRIB_U_W 0x2
#define MEMORY_ATTRIB_U_X 0x4
#define MEMORY_ATTRIB_U_CACHEABLE 0x8
#define MEMORY_ATTRIB_U_ORDERED 0x10
#define MEMORY_ATTRIB_S_R 0x20
#define MEMORY_ATTRIB_S_W 0x40
#define MEMORY_ATTRIB_S_X 0x80
#define MEMORY_ATTRIB_S_CACHEABLE 0x100
#define MEMORY_ATTRIB_S_ORDERED 0x200
#define MEMORY_ATTRIB_U_ALL 0x1F
#define MEMORY_ATTRIB_S_ALL 0x3E0
#define MEMORY_ATTRIB_ALL 0x3FF

/* critical - deterministic
* noncritical - non-deterministic (behavior can change due to status - but not disrupt the output) */
#define MEMORY_ATTRIB_U_CRITICAL 0x7
#define MEMORY_ATTRIB_U_NONCRITICAL 0x18
#define MEMORY_ATTRIB_S_CRITICAL 0xE0
#define MEMORY_ATTRIB_S_NONCRITICAL 0x300
#define MEMORY_ATTRIB_CRITICAL 0xE7
#define MEMORY_ATTRIB_NONCRITICAL 0x318

enum Memory_access_status_enum {none = 0, section_err, attribute_err, reserved};
// value is 0 by default
// if value is 1, no section found
// if value is 2, attribute error
// always reverts to 0 on re-query
extern uint32 Memory_var_access_err;

// functions

// memory init map
// size is in bytes (e.g. 128KB -> size: 0x20000)
extern void Memory_addMap(uint32 base, uint32 size, uint32 attrib);
extern void Memory_init();


// memory read
// size -> 8/16/32 bit
extern void* Memory_read(uint32 addr, Memory_enum_size sizetype, uint32 attrib);

// memory write
// size -> 8/16/32 bit
extern void Memory_write(uint32 addr, Memory_enum_size sizetype, uint32 data, uint32 attrib);


// get memory map
extern Memory_map_elem* Memory_getMap(uint32 addr);
