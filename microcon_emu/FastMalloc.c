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

two-layer allocator structure:
- every allocator has a master bitmap and 32 sub bitmaps, each representing 32 blocks. (so 1024 blocks per allocator)
-- 1 main allocator of 4kb blocks
-- sub allocators for each size classes (512b, 4kb, 64kb, max kb...) with their own bitmaps and memory pools. 
(but their pools are empty at the start, they are only filled when the main allocator gives them blocks)

coalescing ideas:
- dont coalesce.

how to verify the given address on deallocation:
- we can range check with compiled symbols.
-- we can also check map location with it.

thread safety ideas:
- simply divide them per core.
-- with an added benefit of being able to set different allocator pool sizes per core and possibly align them to their local ram.

resource scheduling ideas (to select whether to use bigger blocks or smaller blocks coalesced):
- with the architecture above, we do best-fit allocation for selecting the block size class, and then we do first-fit allocation for selecting the block within the class.



*/

#include "FastMalloc.h"



