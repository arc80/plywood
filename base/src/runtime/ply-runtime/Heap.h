/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/impl/Mutex_LazyInit.h>

namespace ply {

struct HeapStats {
    ureg peakSystemBytes;
    ureg systemBytes;
    ureg inUseBytes;
};

namespace memory_dl {

//-----------------------------------------------------
// Adapted from Doug Lea's malloc: ftp://g.oswego.edu/pub/misc/malloc-2.8.6.c
//
// Note: You can create new heaps by instantiating new
// Heap_t objects, but as long as it exists, you cannot destroy the heap
// in which you instantiated the Heap_t object itself.
//
// This limitation doesn't really matter as long as all your Heap_t objects
// exist at global scope or are allocated in the default heap.
//
// (Doug Lea's original malloc does not have this limitation because it
// represents separate heaps as "mspaces", where the malloc_state is embedded
// the header of a memory region owned by the mspace and passed around by
// pointer.)
//
// The approach chosen here simplifies the implementation of Heap_t, avoids
// any issues with static initialization order, and minimizes runtime overhead.
//-----------------------------------------------------
#define PLY_DLMALLOC_FAST_STATS 0

static const unsigned int NSMALLBINS = (32U);
static const unsigned int NTREEBINS = (32U);

struct malloc_chunk;
typedef struct malloc_chunk mchunk;
typedef struct malloc_chunk* mchunkptr;
typedef struct malloc_chunk* sbinptr; /* The type of bins of chunks */
typedef unsigned int bindex_t;        /* Described below */
typedef unsigned int binmap_t;        /* Described below */
typedef unsigned int flag_t;          /* The type of various bit flag sets */

struct malloc_tree_chunk;
typedef struct malloc_tree_chunk* tbinptr; /* The type of bins of trees */

struct malloc_segment {
    char* base;                  /* base address */
    size_t size;                 /* allocated size */
    struct malloc_segment* next; /* ptr to next segment */
    flag_t sflags;               /* mmap and extern flag */
};
typedef struct malloc_segment msegment;

struct malloc_state {
    binmap_t smallmap;
    binmap_t treemap;
    size_t dvsize;
    size_t topsize;
    char* least_addr;
    mchunkptr dv;
    mchunkptr top;
    size_t trim_check;
    size_t release_checks;
    size_t magic;
    mchunkptr smallbins[(NSMALLBINS + 1) * 2];
    tbinptr treebins[NTREEBINS];
    size_t footprint;
    size_t max_footprint;
    size_t footprint_limit; /* zero means no limit */
    flag_t mflags;
    msegment seg;
    void* extp; /* Unused but available for extensions */
    size_t exts;
#if PLY_DLMALLOC_FAST_STATS
    size_t inUseBytes;
#endif
};
typedef struct malloc_state* mstate;

} // namespace memory_dl

class Heap_t {
private:
    memory_dl::malloc_state mstate;
    Mutex_LazyInit mutex;

public:
    // If you create a Heap_t at global scope, it will be automatically zero-init. Otherwise, you should call this function before using it:
    void zeroInit();
    void* alloc(ureg size);
    void* realloc(void* ptr, ureg newSize);
    void free(void* ptr);
    void* allocAligned(ureg size, ureg alignment);
    void freeAligned(void* ptr);
    HeapStats getStats();
    ureg getInUseBytes() const;
    ureg getSize(void* ptr);
};

PLY_DLL_ENTRY extern Heap_t Heap;

} // namespace ply
