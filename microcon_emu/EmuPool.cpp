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
 * [0x2002] = 0xXXXX (last byte of this block)
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
 * allocation always goes left -> right. it does not go reverse. we can use this fact to indicate an empty gap.
 * when we dealloc block B:
 * block A -> block B -> block C
 * block A -> update to point to block C
 * block C -> do not update.(points to nonexistant block B)
 * when checking for gap, we can use this to calculate gap size, and insert if possible.
 */

uint32 microcon_emupool[EMUPOOL_BUFFER_SIZE];
void* Proxy_allocate_memory(uint32 size) {
	uint32 index = 0;
	uint32 blocksize = size / 4 + 3;	// size in 4byte + 3 for header

	uint32 curr_header_prevblock_startaddr = 0;
    uint32 curr_header_nextblock_startaddr = 0;
    uint32 prev_header_prevblock_startaddr = 0;
	uint32 prev_header_nextblock_startaddr = 0;
    uint32 next_header_prevblock_startaddr = 0;
    uint32 next_header_nextblock_startaddr = 0;

    // very first allocation
    if (microcon_emupool[0] == 0){
        microcon_emupool[0] = 0xAAAAAAAA; // magic number
        microcon_emupool[1] = 0; // previous block is null

        microcon_emupool[2] = blocksize; // next block starts after this block
        microcon_emupool[blocksize] = 0xAAAAAAAA; // magic number for next block
        microcon_emupool[blocksize + 1] = 0; // next block's previous pointer is null

        return &microcon_emupool[3];
    }

    // we have at least one block here
	while(1){
        if (microcon_emupool[index] != 0xAAAAAAAA){
            // memory corruption detected
            m_assert(0, "memory corruption detected in emu pool");
        }

		if (index + blocksize + 3/* next (possible)block's header */ >= EMUPOOL_BUFFER_SIZE) {
			// out of memory
			m_assert(0, "out of memory in emu pool");
		}

		/* current block's header */
		curr_header_prevblock_startaddr = microcon_emupool[index + 1];
		curr_header_nextblock_startaddr = microcon_emupool[index + 2];
        next_header_prevblock_startaddr = microcon_emupool[curr_header_nextblock_startaddr + 1];
        next_header_nextblock_startaddr = microcon_emupool[curr_header_nextblock_startaddr + 2];

        /* one iteration */
        /* check if we are at the position of the last + 1 block */
        if (curr_header_prevblock_startaddr != 0x0 && curr_header_nextblock_startaddr == 0x0){
            /* start adding new block */
            microcon_emupool[index + 2] = index + blocksize;
            /* pre-init for next block */
            microcon_emupool[index + blocksize] = 0xAAAAAAAA;
            microcon_emupool[index + blocksize + 1] = index;
            return &microcon_emupool[index + 3];
        }
        /* check if there is a free block between previous block and current block */
        else if (curr_header_prevblock_startaddr != 0x0 && curr_header_nextblock_startaddr != 0x0){
            /* when dealloc, the next block's prevaddr is not updated. we use that to figure out the size */
            if ()
        }


		/* update index to the next block */
		index = curr_header_nextblock_startaddr;

		/* update previous entry info */
		prev_header_prevblock_startaddr = curr_header_prevblock_startaddr;
		prev_header_nextblock_startaddr = curr_header_nextblock_startaddr;

	}


}

void Proxy_free_memory(void* ptr){

}
