#pragma once

#include "Proxy.hpp"

// basic memory map for armv7m emulation
/* this is not meant to be an interface for the debugger. this is only for the base cpu and peripherals */
/* we need another separate subsystem for emulating jtag and debugging */
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

/*
* section for peripheral memory access
*
* every cycle is scheduled (since its supposed to be a cycle accurate emulator)
* just like cpu, peripheral cycle is also explicit, meaning it will always run and have time to update in realtime.
* making this subsystem as an interrupt scheduling will be very complex.
* so we will make it a one time access flag system. (and make the access a queue per address)
* (make a callback function registerable too)
*
* - right now the memory map is maintained with a list of separately allocated sections.
* we shall add an operation queue head per section Memory_map_elem->opqueue
* when the cpu reads / writes to memory -> simple
* when the cpu reads / writes to peripheral memory -> not simple; depending on specs, the write may be ignored
* or the read data may be different depending on context(user/super)
*
* - we will have multiple peripherals operating on the same section. what to do with a single queue?
* - we can't do queue per memory address. thats going to consume massive resource.
* - however, we can do queue per peripherals.
* we shall make a separate pointer that holds a map of peripheral indexes that has been registered in that section.
*
* 1. when launching, each peripherals shall register themselves into that map with specific indexes.
* - Memory_peri_register(index, array of addresses that the peri uses, read callback function)
*
* 2. while running, cpu access the peripheral section:
* - Memory_write(addr, sizetype, data, attrib) -> check attrib of the section (its a peripheral) -> Memory_write_peri(addr, sizetype, data)
* -> linear search over peri register map (check address) and find match -> Memory_queue_peri(peri_idx, addr, sizetype, data)
* -> it is queued in linear fashion.
*
* - Memory_read(addr, sizetype, attrib) -> check attrib of the section -> Memory_read_peri(addr, sizetype, data)
* -> linear search over peri register map (check address) and find match -> read callback is executed 
* -> may check cpu state, return value, writeback to memory via Memory_write(with super attrib)
* (no queue access for reads)
*
* 3. while running, peripheral handle the write queue:
* - peripheral code shall call Memory_handle_writequeue_peri(peri_idx, write callback function) 
* -> this function iterates the queue of the peri and calls the write callback with the parameter (addr, sizetype, data, attrib) per iteration
* -> and the callback function decides whether to write or nay using Memory_write(with super attrib)
*/


/*
* TODOs:
* - address race conditions of the memory access when we are to implement multicore in this emulator.
* - bus error if wrongly accessed in peripheral section
* - sync op for readback (with the current design, we dont have that)
* - 
*/

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
