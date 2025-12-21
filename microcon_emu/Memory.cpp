#include "Memory.hpp"

Memory_map_elem Memory_var_arr[MEMORY_MAP_MAX_SECTIONS];
uint32 Memory_var_arrlen = 0;
uint32 Memory_var_access_err = 0;

uint32 Memory_var_endianness = 0;

// memory init map

void Memory_addMap(uint32 base, uint32 size, uint32 attrib) {
	if (Memory_var_arrlen == 0) {
		// first add
		Memory_var_arr[Memory_var_arrlen].next = NULL;
		Memory_var_arr[Memory_var_arrlen].size = size;
		Memory_var_arr[Memory_var_arrlen].base = base;
		Memory_var_arr[Memory_var_arrlen].attrib = attrib & MEMORY_ATTRIB_CRITICAL;
		Memory_var_arr[Memory_var_arrlen].nd_attrib = attrib & MEMORY_ATTRIB_NONCRITICAL;
		Memory_var_arr[Memory_var_arrlen].data = (uint8*)ecalloc(size, sizeof(uint8));
	
	}
	else if (Memory_var_arrlen == MEMORY_MAP_MAX_SECTIONS){
		// last add
		printf("Memory section full!\n");
		return;
	}
	else {
		// middle
		Memory_var_arr[Memory_var_arrlen].next = NULL;
		Memory_var_arr[Memory_var_arrlen - 1].next = &Memory_var_arr[Memory_var_arrlen];
		Memory_var_arr[Memory_var_arrlen].size = size;
		Memory_var_arr[Memory_var_arrlen].base = base;
		Memory_var_arr[Memory_var_arrlen].attrib = attrib & MEMORY_ATTRIB_CRITICAL;
		Memory_var_arr[Memory_var_arrlen].nd_attrib = attrib & MEMORY_ATTRIB_NONCRITICAL;
		Memory_var_arr[Memory_var_arrlen].data = (uint8*)ecalloc(size, sizeof(uint8));
	
	}

	Memory_var_arrlen += 1;


}
void Memory_init() {

	Memory_var_endianness = 0;
	Memory_var_arrlen = 0;
	Memory_var_access_err = 0;
	// example
	/* we ignore cache for now
	* <basic map>
	* 0x0 ~ 0x1FFFFFFF: Code (vector table + ROM)
	* 0x20000000 ~ 0x3FFFFFFF: SRAM
	* 0x40000000 ~ 0x5FFFFFFF: Peripheral
	* 0x60000000 ~ 0xDFFFFFFF: Some sh*t
	* 0xE0000000 ~ 0xFFFFFFFF: PPB
	* total 5 maps to produce
	*/

	// Code
	Memory_addMap(0x0, 0x20000, MEMORY_ATTRIB_S_ALL);
	// SRAM
	Memory_addMap(0x20000000, 0x20000, MEMORY_ATTRIB_S_ALL);
	// Peripheral
	Memory_addMap(0xE0000000, 0x20000000, MEMORY_ATTRIB_S_ALL);

	Memory_init_peri(); // temporary. delete this later.

}

void systick(){
	// do something here
}

void Memory_init_peri(){
	// systick
	struct peri_addr_t temp;
	temp.peri_arr = emalloc(4, sizeof(uint32));
	temp.peri_arr_size = 4;
	temp.peri_arr[0] = 0xE000E010; // SYST_CSR
	temp.peri_arr[1] = 0xE000E014; // SYST_RVR
	temp.peri_arr[2] = 0xE000E018; // SYST_CVR
	temp.peri_arr[3] = 0xE000E01C; // SYST_CALIB

	Memory_addPeri(0, &temp, &systick);
}

void Memory_addPeri(uint32 idx, struct peri_addr_t* peri_addr, void (*readcallback)(void)){
	// find allocated map from the maplist
	
}

Memory_map_elem* Memory_getMap(uint32 addr) {
	if (Memory_var_arrlen > 0) {
		Memory_map_elem* item = &Memory_var_arr[0];
		do {
			uint32 base = item->base;	// inclusive
			uint32 bound = item->base + item->size;	// exclusive
			
			if (addr >= base && addr <= bound) {
				return item;
			}
		
			item = item->next;
		} while (item != NULL);
	
	}
	else {
		printf("no map exists\n");
		return NULL;
	}

	return NULL;	// no valid map exists
}

// memory read
// size -> 8/16/32 bit
void* Memory_read(uint32 addr, Memory_enum_size sizetype, uint32 attrib) {
	Memory_map_elem* thismap;

	Memory_var_access_err = 0;

	// get memory section
	if ((thismap = Memory_getMap(addr)) == NULL) {
		// fail if NULL is returned (invalid address)
		Memory_var_access_err = Memory_access_status_enum::section_err;
		return NULL;
	}

	// get memory section attribute
	if ((thismap->attrib & (attrib & MEMORY_ATTRIB_CRITICAL)) == 0) {
		// fail if attribute error
		Memory_var_access_err = Memory_access_status_enum::attribute_err;
		return NULL;
	}

	// get memory section nd_attrib
	if ((thismap->nd_attrib & (attrib & MEMORY_ATTRIB_NONCRITICAL)) == 0) {
		// do nothing
		// TODO: do something... like simulate a delay or order of output
	}

	uint32 translated_addr = addr - thismap->base;

	//little-endian
	if (Memory_var_endianness == 0) {
		return &thismap->data[translated_addr];
	}
	else {
		// big endian - do not support
		/*switch (sizetype) {
		case Memory_enum_size::u8:
			
			break;
		case Memory_enum_size::u16:
			break;
		case Memory_enum_size::u32:
			break;
		default:
			break;
		}
		return &thismap->data[translated_addr];*/
	}

	// what happen?
	return NULL;
}

// memory write
// size -> 8/16/32 bit
void Memory_write(uint32 addr, Memory_enum_size sizetype, uint32 data, uint32 attrib) {
	Memory_map_elem* thismap;

	Memory_var_access_err = 0;

	// get memory section
	if ((thismap = Memory_getMap(addr)) == NULL) {
		// fail if NULL is returned (invalid address)
		Memory_var_access_err = Memory_access_status_enum::section_err;
		return;
	}

	// get memory section attribute
	if ((thismap->attrib & (attrib & MEMORY_ATTRIB_CRITICAL)) == 0) {
		// fail if attribute error
		Memory_var_access_err = Memory_access_status_enum::attribute_err;
		return;
	}

	// get memory section nd_attrib
	if ((thismap->nd_attrib & (attrib & MEMORY_ATTRIB_NONCRITICAL)) == 0) {
		// do nothing
		// TODO: do something... like simulate a delay or order of output
	}

	uint32 translated_addr = addr - thismap->base;
	//little-endian
	if (Memory_var_endianness == 0) {

		switch (sizetype) {
		case Memory_enum_size::u8:

			thismap->data[translated_addr] = (uint8)(data & 0xFF);
			break;
		case Memory_enum_size::u16:
			
			thismap->data[translated_addr] = (uint8)(data & 0xFF);	// first 8bit
			thismap->data[translated_addr + 1] = (uint8)((data >> 8) & 0xFF); // second 8bit
			break;
		case Memory_enum_size::u32:
			thismap->data[translated_addr] = (uint8)(data & 0xFF);	// first 8bit
			thismap->data[translated_addr + 1] = (uint8)((data >> 8) & 0xFF); // second 8bit
			thismap->data[translated_addr + 2] = (uint8)((data >> 16) & 0xFF); // third 8bit
			thismap->data[translated_addr + 3] = (uint8)((data >> 24) & 0xFF); // fourth 8bit
			break;
		default:
			break;
		}

		
	}


}

