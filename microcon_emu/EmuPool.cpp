#include "EmuPool.hpp"
/* simple memory allocator for emulation structures
 * structure:
 * 1MB buffer
 * first 4byte is a magic number for corruption detection (0xAAAAAAAA)
 * second 4byte is a pointer of ending address + 3
 * if a middle block is freed, previous block's next pointer is set to the next of the freed block
 * freed block's next pointer is set to nullptr
 * ending address + 1 is the address to the first byte of previous block (needed to traverse backwards)
 * so: for the single block, [0] is a magic number, [1] is previous block's starting address, [2] is next block's starting address, [3] is first byte of this block.
 *
 * for example: lets say we allocate 3 blocks of 0x2000 bytes each
 * - first block
 * [0] = 0xAAAAAAAA (magic number for corruption detection)
 * [1] = (previous block's starting address; which should be 0 if this is the first block)
 * [2] = 0x2003 (next block's starting address; 0x2000 + 3 for this block's header)
 * [3] = 0xXXXX (first byte of this block)
 * ...
 * [0x2002] = 0xXXXX (last byte of this block; not 0x1FFF because we need to store the header of the block which is + 2)
 * - second block
 * [0x2003] = 0xAAAAAAAA
 * [0x2004] = 0x0 (previous block's starting address)
 * [0x2005] = 0x4006 (next block's starting address)
 * [0x2006] = 0xYYYY (first byte of this block)
 * ...
 * [0x4005] = 0xYYYY (last byte of this block)
 * - third block
 * [0x4006] = 0xAAAAAAAA
 * [0x4007] = 0x2004 (previous block's starting address)
 * [0x4008] = 0x6009 (next block's starting address; points to a place that doesn't exist, but is done anyway to indicate the end of the pool)
 * [0x4009] = 0xZZZZ (first byte of this block)
 * ...
 * [0x6008] = 0xZZZZ (last byte of this block)
 * - no blocks after this point
 * [0x6009] = 0xAAAAAAAA (assigned when inserting the third block)
 * [0x600A] = 0x4007 (previous block's starting address; assigned when inserting the third block)
 * [0X600B] = 0x0
 * ...
 *  - that's it.
 * 
 * when allocating freed blocks, we can check for the free blocks by checking the [1] of the next block and [0] of the current block.
 * if the difference is not equal to the size of the block + 2, it means there is a free block in between.
 */

uint32 microcon_emupool[EMUPOOL_BUFFER_SIZE];
void* Proxy_allocate_memory(uint32 size) {
	uint32 index = 0;
	uint32 blocksize = size / 4 + 2;	// size in 4byte + 2 for header

	uint32 prev_header_prevblock_startaddr = 0;
	uint32 prev_header_nextblock_startaddr = 0;

	while(1){
		if (index + blocksize >= EMUPOOL_BUFFER_SIZE) {
			// out of memory
			m_assert(0, "out of memory in emu pool");
		}

		/* current block's header */
		uint32 header_prevblock_startaddr = microcon_emupool[index];
		uint32 header_nextblock_startaddr = microcon_emupool[index + 1];

		/* if we are inserting a very first block */
		if(header_prevblock_startaddr == 0 && header_nextblock_startaddr == 0){
			/* set header */
			microcon_emupool[index] = 0;	// previous block is null
			microcon_emupool[index + 1] = blocksize;	// next block starts after this block

			/* set next block's previous pointer */
			microcon_emupool[blocksize + 1] = 0;	// next block's previous pointer points to this block (oom is already considered at the start of the loop)

			/* return pointer to the first byte of this block */
			return &microcon_emupool[index + 2];
		}

		/* we are inserting block normally. */

		/* check if this block is first entry in pool */
		else if (header_prevblock_startaddr == 0){

		}



		/* update index to the next block */
		index = header_nextblock_startaddr;

		/* update previous entry info */
		prev_header_prevblock_startaddr = header_prevblock_startaddr;
		prev_header_nextblock_startaddr = header_nextblock_startaddr;

	}


}

void Proxy_free_memory(void* ptr){

}
