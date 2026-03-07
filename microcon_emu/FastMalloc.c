/*
super simple yet super fast memory allocator for non mmu systems

requirements:
- needs to have closed to fixed time for allocation (RTOS requirement)
- needs less fragmentation
- simplicity (otherwise debugging will be a nightmare)
- must be thread safe AND performant at the same time.

basic ideas:
- separate containers per size (array for 512b, 4kb, 64kb, max kb...)
-- we can reduce fragmentation
- simple bitmaps for each containers.
-- we can count the leading zeroes of the bitmap using special opcodes
-- 32bit bitmap is not enough, so lets use dual layers (1 master map + 32 sub maps = 33 32bit variables = representing 1024 slots)
--- master map is only bit flipped when the corresponding sub map is completely filled.
--- on dealloc, we only need to bitflip the master to 0 without any other checks.

search ideas:
- simple for loop iteration to the end for systems with no special opcodes.
- use clz for systems that support them.
-- good for first searches.
-- need a different way for once-freed blocks. (needs critical thinking)
--- use map inversion and check once again using the clz to find the 1 bit?

coalescing ideas:
- we keep at least one 32bit variable before the memory block to track its:
-- link with next block (should be a count of consecutive blocks for coalescing - other coalesced blocks shall not have this variable.)
--- on alloc, the coalesced blocks will also have sub map bits flipped to 1 (to prevent alloc selection)
- we return the block address + 1 to user.
- on dealloc - with the address, we use the compiled symbol to find its actual bit location on master and sub maps.
-- we dealloc other linked blocks by checking the 32bit variable.

how to verify the given address on deallocation:
- we can range check with compiled symbols.
-- we can also check map location with it.
- if the address given is -1 (seeing the config var) - we shall just mask the first 2 bits of the address by default and +1.
-- (but then less than 16b block is not supported)
- if the address given is pointing to coalesced block - we shall make an extra allocator map structure (33 maps) just for the coalescing block info.
-- so, 2x 33 32bit variables of allocator structure per core.

thread safety ideas:
- simply divide them per core.
-- with an added benefit of being able to set different allocator pool sizes per core and possibly align them to their local ram.


resource scheduling ideas (to select whether to use bigger blocks or smaller blocks coalesced):
- with the architecture above, 


*/

#include "FastMalloc.h"



