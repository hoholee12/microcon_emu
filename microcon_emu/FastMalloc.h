#include "Proxy.hpp"

/* types & structures */

/* vma template
- for main allocator, we will only use one vma that covers the whole memory pool.
- for sub allocators, we will have multiple vmas
*/
typedef struct {
    uint32* pool; /* pointer to the memory pool */
    uint32 size; /* size of the memory pool */
    vmat* next; /* pointer to the next vma */
} vmat;

/* allocator template */
typedef struct {
    uint32 fastmap; /* bitmap */
    uint32 submap[32]; /* bitmap for each 32 blocks */
    uint32 fixedsize; /* total allowed size; must be fixed to MAXSIZE */
    uint32 cursize; /* current allocated size */
    vmat* vmalist; /* list of vmas */
} malloct;



/* to be - generated macros & variables */


#define MAXSIZE 0x100000 /* 1mb max size for the whole allocator */



/* prototypes */


malloct main_allocator; /* the main allocator */
malloct sub_allocators[5]; /* the sub allocators for each size class (512b, 4kb, 64kb, max kb...) */



/* functions */
void init_allocator(uint32 fixedsize);
void* fast_malloc(uint32 size);
void fast_free(void* ptr);
