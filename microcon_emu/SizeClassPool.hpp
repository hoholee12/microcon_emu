#pragma once
#include "Proxy.hpp"
#include <string.h>
#include "EmuPool.hpp"

/* size class allocator
 * manage memory in size classes
 * ideas:
 * - make sure alloc pool is aligned to biggest size class
 * - make sure each size class is aligned to its own size class
 * - to do this we need to allocate the biggest size class first, then the next biggest, and so on
 * - with this we can guarantee that each size class is aligned to its own size class.
 * 
 * ideas for allocation:
 * - keep in mind header exists per block, so we need to account for that in alignment
 * - use median sentinel - modify the sentinel distance to be median of the size class, so that we can guarantee that each size class is aligned to its own size class.
 * - and we can use those sentinels as a head for each size class.
 * (for that we need to modify emupool to expose index of the median sentinels - TODO)
 * 
 *  */