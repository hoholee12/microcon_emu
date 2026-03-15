/*
super simple yet super fast memory allocator for non mmu systems

requirements:
- needs to have closed to fixed time for allocation (RTOS requirement)
- needs less fragmentation
- simplicity (otherwise debugging will be a nightmare)
- must be thread safe AND performant at the same time.

basic ideas:
- separate classes per size (array for 512b, 4kb, 64kb, max kb...)
-- we can reduce fragmentation
- simple bitmaps for each containers.
-- we can count the leading zeroes of the bitmap using special opcodes
-- 32bit bitmap is not enough, so lets use dual layers (1 master map + 32 sub maps = 33 32bit variables = representing 1024 slots)
--- master map is only bit flipped when the corresponding sub map is completely filled. (checked by comparing the sub map with 0xFFFFFFFF)
--- on dealloc, we only need to bitflip the master to 0 without any other checks.

search ideas:
- simple for loop iteration to the end for systems with no special opcodes.
-- use clz or similar opcode for systems that support them.
--- good for first searches.
--- for once-freed blocks, we need a different approach.
---- 1. do the normal clz search (greedy approach). the location is the clz result.
---- 2. if clz returns 0 & master bit is still 0, invert the sub map and do clz again to find the freed block. the location is the clz result + 1.

armv7m: supports mvn(bitwise not for inversion) and clz(count leading zeroes)
rh850: supports sch0l(search 0 starting from msb) - this essentially emulates a full hardware for loop. (no extra logic needed for the once-freed blocks)
x86: supports bsf(bit scan forward)

two-layer allocator structure:
- every allocator has a master bitmap and 32 sub bitmaps, each representing 32 blocks. (so 1024 blocks per allocator)
-- 1 main allocator of 4kb blocks (not the smallest nor the biggest, but a middle ground)
-- sub allocators for each size classes (512b, 4kb, 64kb, max kb...) with their own bitmaps and memory pools. 
(but their pools are empty at the start, they are only filled when the main allocator gives them blocks)

coalescing ideas for sub allocator:
- lets say the sub allocator is for 64kb blocks.
- the allocator requests sequential blocks from the main allocator to be able to coalesce them into a 64kb block.

splitting ideas for sub allocator:
- lets say the sub allocator is for 512b blocks.
- the allocator must request one 4kb block from the main allocator
-- and split it into 8 512b blocks.
- on allocation
-- we check if the sub allocator has any blocks available
--- if not, we can request a block from the main allocator
(we keep track of the available size in the sub allocator <in one variable> to know when we need to request more blocks from the main allocator)

how to verify the given address on deallocation:
- we can range check with compiled symbols.
-- we can also check map location with it.

thread safety ideas:
- simply divide them per core.
-- with an added benefit of being able to set different allocator pool sizes per core and possibly align them to their local ram.

resource scheduling ideas (to select whether to use bigger blocks or smaller blocks coalesced):
- with the architecture above, we do best-fit allocation for selecting the block size class
-- and the class allocator will do the best fit search for the block within its class. 
(so we have a two level best fit allocation)
--- if we dont have any blocks in the class, we can try to get a block from the main allocator.
(with this design, theres no splitting nor coalescing)
--- we need to keep an extra variable to track the allocated size of the whole allocator. 
(so that the sub allocators can check if they have enough space to allocate a block from the main allocator)

freeing ideas for the main allocator:
- we design the sub allocators to simply actively(on free call) free the blocks back to the main allocator. (to keep the main allocator from being exhausted)
-- then we wont need to keep track of whether we already have the main allocator blocks allocated to the sub allocators.

*/

/*
prototype:



*/

#include "FastMalloc.h"



void init_allocator(uint32 fixedsize) {
    // initialize the main allocator
    main_allocator.fixedsize = fixedsize;
    main_allocator.cursize = 0;
    main_allocator.fastmap = 0;
    for (int i = 0; i < 32; i++) {
        main_allocator.submap[i] = 0;
    }
    main_allocator.pool = (uint32*)malloc(fixedsize);
    main_allocator.vmalist = (vmat*)malloc(sizeof(vmat));
    main_allocator.vmalist->pool = main_allocator.pool;
    main_allocator.vmalist->size = fixedsize;
    main_allocator.vmalist->next = NULL;

    // initialize the sub allocators
    for (int i = 0; i < 5; i++) {
        sub_allocators[i].fixedsize = 0; // we will set this later based on the size class
        sub_allocators[i].cursize = 0;
        sub_allocators[i].fastmap = 0;
        for (int j = 0; j < 32; j++) {
            sub_allocators[i].submap[j] = 0;
        }
        sub_allocators[i].pool = NULL; // we will set this when we request blocks from the main allocator
        sub_allocators[i].vmalist = NULL; // we will set this when we request blocks from the main allocator
    }
}
