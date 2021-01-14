/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/impl/Mutex_LazyInit.h>
#include <string.h>

namespace ply {
namespace memory_dl {

struct Stats {
    ureg peakSystemBytes;
    ureg systemBytes;
    ureg inUseBytes;
};

//-----------------------------------------------------
// Adapted from Doug Lea's malloc: ftp://g.oswego.edu/pub/misc/malloc-2.8.6.c
//
// Note: You can create new heaps by instantiating new
// Heap_DL objects, but as long as it exists, you cannot destroy the heap
// in which you instantiated the Heap_DL object itself.
//
// This limitation doesn't really matter as long as all your Heap_DL objects
// exist at global scope or are allocated in the default heap.
//
// (Doug Lea's original malloc does not have this limitation because it
// represents separate heaps as "mspaces", where the malloc_state is embedded
// the header of a memory region owned by the mspace and passed around by
// pointer.)
//
// The approach chosen here simplifies the implementation of Heap_DL, avoids
// any issues with static initialization order, and minimizes runtime overhead.
//-----------------------------------------------------
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

PLY_DLL_ENTRY void* dlmalloc(size_t, mstate);
PLY_DLL_ENTRY void dlfree(void*, mstate);
PLY_DLL_ENTRY void* dlcalloc(size_t, size_t, mstate);
PLY_DLL_ENTRY void* dlrealloc(void*, size_t, mstate);
PLY_DLL_ENTRY void* dlrealloc_in_place(void*, size_t, mstate);
PLY_DLL_ENTRY void* dlmemalign(size_t, size_t, mstate);
PLY_DLL_ENTRY int dlposix_memalign(void**, size_t, size_t, mstate);
PLY_DLL_ENTRY void* dlvalloc(size_t, mstate);
PLY_DLL_ENTRY int dlmallopt(int, int, mstate);
PLY_DLL_ENTRY size_t dlmalloc_footprint(mstate);
PLY_DLL_ENTRY size_t dlmalloc_max_footprint(mstate);
PLY_DLL_ENTRY size_t dlmalloc_footprint_limit(mstate);
PLY_DLL_ENTRY size_t dlmalloc_set_footprint_limit(size_t bytes, mstate);
PLY_DLL_ENTRY void** dlindependent_calloc(size_t, size_t, void**, mstate);
PLY_DLL_ENTRY void** dlindependent_comalloc(size_t, size_t*, void**, mstate);
PLY_DLL_ENTRY size_t dlbulk_free(void**, size_t n_elements, mstate);
PLY_DLL_ENTRY void* dlpvalloc(size_t, mstate);
PLY_DLL_ENTRY int dlmalloc_trim(size_t, mstate);
PLY_DLL_ENTRY void dlmalloc_stats(mstate, Stats&);
PLY_DLL_ENTRY size_t dlmalloc_usable_size(void*);
//-----------------------------------------------------

} // namespace memory_dl

class Heap_DL {
private:
    memory_dl::malloc_state m_mstate;
    Mutex_LazyInit m_mutex;

public:
    // If you create a Heap_DL at global scope, it will be automatically
    // zero-init.
    // Otherwise, you should call this function before using it:
    void zeroInit() {
        memset(static_cast<void*>(this), 0, sizeof(*this));
    }

    typedef memory_dl::Stats Stats;

    class Operator {
    private:
        Heap_DL& m_mem;

    public:
        Operator(Heap_DL& mem) : m_mem(mem) {
        }

        // There may also be extra indirection/checks inside the functions
        PLY_NO_INLINE void* alloc(ureg size) {
            LockGuard<Mutex_LazyInit> guard(m_mem.m_mutex);
            return memory_dl::dlmalloc((size_t) size, &m_mem.m_mstate);
        }

        PLY_NO_INLINE void* realloc(void* ptr, ureg newSize) {
            LockGuard<Mutex_LazyInit> guard(m_mem.m_mutex);
            return memory_dl::dlrealloc(ptr, (size_t) newSize, &m_mem.m_mstate);
        }

        PLY_NO_INLINE void free(void* ptr) {
            LockGuard<Mutex_LazyInit> guard(m_mem.m_mutex);
            return memory_dl::dlfree(ptr, &m_mem.m_mstate);
        }

        PLY_NO_INLINE void* allocAligned(ureg size, ureg alignment) {
            LockGuard<Mutex_LazyInit> guard(m_mem.m_mutex);
            return memory_dl::dlmemalign((size_t) alignment, (size_t) size, &m_mem.m_mstate);
        }

        void freeAligned(void* ptr) {
            free(ptr);
        }

        PLY_NO_INLINE Stats getStats() {
            Stats stats;
            LockGuard<Mutex_LazyInit> guard(m_mem.m_mutex);
            memory_dl::dlmalloc_stats(&m_mem.m_mstate, stats);
            return stats;
        }

#if PLY_DLMALLOC_FAST_STATS
        ureg getInUseBytes() const {
            return m_mem.m_mstate.inUseBytes;
        }
#endif
    };

    ureg getSize(void* ptr) {
        // No need to lock
        return memory_dl::dlmalloc_usable_size(ptr);
    }

    Operator operate(const char*) {
        return Operator(*this);
    }
};

} // namespace ply
