#include "Memory.hpp"

Memory_map_elem Memory_var_arr[10];
uint32 Memory_var_arrlen = 0;

uint32 Memory_var_endianness = 0;

// memory init map
void Memory_addMap(uint32 base, uint32 size) {
	if (Memory_var_arrlen == 0) {
		// first add
		Memory_var_arr[Memory_var_arrlen].next = NULL;
		Memory_var_arr[Memory_var_arrlen].size = size;
		Memory_var_arr[Memory_var_arrlen].base = base;
		Memory_var_arr[Memory_var_arrlen].data = (uint8*)calloc(size, sizeof(uint8));
	
	}
	else if (Memory_var_arrlen >= 9){
		// last add
		
		return;
	}
	else {
		// middle
		Memory_var_arr[Memory_var_arrlen].next = NULL;
		Memory_var_arr[Memory_var_arrlen - 1].next = &Memory_var_arr[Memory_var_arrlen];
		Memory_var_arr[Memory_var_arrlen].size = size;
		Memory_var_arr[Memory_var_arrlen].base = base;
		Memory_var_arr[Memory_var_arrlen].data = (uint8*)calloc(size, sizeof(uint8));
	
	}

	Memory_var_arrlen += 1;


}
void Memory_init() {

	Memory_var_endianness = 0;
	Memory_var_arrlen = 0;
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
	Memory_addMap(0x0, 0x20000);
	// SRAM
	Memory_addMap(0x20000000, 0x20000);



}

Memory_map_elem* Memory_getMap(uint32 addr) {
	uint32 validMapFound = 0;
	if (Memory_var_arrlen > 0) {
		Memory_map_elem* item = &Memory_var_arr[0];
		do {
			uint32 base = item->base;	// inclusive
			uint32 bound = item->base + item->size;	// exclusive
			
			if (addr >= base && addr <= bound) {
				validMapFound = 1;
				return item;
			}
		
			item = item->next;
		} while (item != NULL);
	
	}
	else {
		printf("no map exists\n");
		return NULL;
	}

	if (validMapFound == 0) {
		return NULL;
	}

}

// memory read
// size -> 8/16/32 bit
void* Memory_read(uint32 addr, Memory_enum_size sizetype) {

	Memory_map_elem* thismap;
	if ((thismap = Memory_getMap(addr)) == NULL) {
		// fail if NULL is returned (invalid address)
		printf("invalid address access!\n");
		return NULL;
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

}

// memory write
// size -> 8/16/32 bit
void Memory_write(uint32 addr, Memory_enum_size sizetype, uint32 data) {
	Memory_map_elem* thismap;
	if ((thismap = Memory_getMap(addr)) == NULL) {
		// fail if NULL is returned (invalid address)
		printf("invalid address access!\n");
		return;
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

