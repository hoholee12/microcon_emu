#include "Proxy.hpp"

/* simple memory allocator for emulation structures
 * structure:
 * 1MB buffer
 * first 4byte is a pointer of ending address + 2 (usually another block's pointer with next ending address + 2 or a nullptr)
 * if a middle block is freed, previous block's next pointer is set to the next of the freed block
 * freed block's next pointer is set to nullptr
 * ending address + 1 is the address to the first byte of previous block (needed to traverse backwards)
 * so: for the single block, [0] is previous block's starting address, [1] is next block's starting address, [2] is first byte of this block.
 * 
 * for example: lets say we allocate 3 blocks of 0x2000 bytes each
 * - first block
 * [0] = should be nullptr if this is the first block
 * [1] = 0x2002 (next block's starting address; not 0x2000 because we need to store the header of the block which is + 2)
 * [2] = 0xXXXX (first byte of this block)
 * ...
 * [0x2001] = 0xXXXX (last byte of this block; not 0x1FFF because we need to store the header of the block which is + 2)
 * - second block
 * [0x2002] = 0x0 (previous block's starting address)
 * [0x2003] = 0x4004 (next block's starting address)
 * [0x2004] = 0xYYYY (first byte of this block)
 * ...
 * [0x4003] = 0xYYYY (last byte of this block)
 * - third block
 * [0x4004] = 0x2002 (previous block's starting address)
 * [0x4005] = 0x0 (next block's starting address; would be 0x6006 if there was a next block, but since this is the last block, it is set to nullptr)
 * [0x4006] = 0xZZZZ (first byte of this block)
 * ...
 * [0x6005] = 0xZZZZ (last byte of this block)
 * [0x6006] = 0x0
 * [0x6007] = 0x0
 * ...
 *  - that's it.
 */
uint32 microcon_emupool[EMUPOOL_BUFFER_SIZE];
void* Proxy_allocate_memory(uint32 size) {
	for (uint32 i = 0; i < EMUPOOL_BUFFER_SIZE;) {
		// check if this block is free
		if (microcon_emupool[i] == 0 && microcon_emupool[i + 1] == 0) {
			// check if size fits
			if (i + size / 4 + 2 > EMUPOOL_BUFFER_SIZE) {
				// does not fit here, check next possible hole
				i += microcon_emupool[i + 1] - i; // skip to next block
				continue;
			}
			else {
				// fits, allocate
				// set previous block pointer
				microcon_emupool[i] = (i == 0) ? 0 : i - microcon_emupool[i - 1]; // if first block, set to nullptr
				// set next block pointer
				microcon_emupool[i + 1] = (i + size / 4 + 2 >= EMUPOOL_BUFFER_SIZE) ? 0 : i + size / 4 + 2; // if last block, set to nullptr
				// set end pointer of this block
				microcon_emupool[i + size / 4 + 1] = i; // point to starting address of this block

				// return pointer to first byte of this block
				return &microcon_emupool[i + 2];
			}
		}
		else {
			// if only one of the two is zero, it means its a corrupted block
			m_assert(microcon_emupool[i + 1] != 0, "\n");
			// not a used block, skip to next block
			i += microcon_emupool[i + 1] - i; // next block's starting address - current block's starting address
			m_assert(i < EMUPOOL_BUFFER_SIZE, "emulator ran out of pool memory.\n");
		}
	}

	return microcon_emupool;
}

void Proxy_free_memory(void* ptr){

}

DWORD WINAPI ThreadFunc(void* data) {
	// thread function. call Core_mainThread here.
	Thread_data* mydata = (Thread_data*)data;


	// core runs here
	mydata->func();


	return 0;
}

HANDLE make_thread(Thread_data* mydata) {

	HANDLE thread;

	// thread attrib, stacksize, funcaddr, param, creationflag, tid
	thread = CreateThread(NULL, 0, ThreadFunc, (void*)mydata, 0, NULL);


	return thread;
}

uint32 Clock_gettime_msec() {
	return GetTickCount();
}

void Clock_sleep(uint32 msec) {
	Sleep(msec);
}
