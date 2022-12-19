/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include <ply-runtime/Precomp.h>
#include <ply-runtime/Heap.h>

namespace ply {
namespace memory_dl {

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
PLY_DLL_ENTRY void dlmalloc_stats(mstate, HeapStats&);
PLY_DLL_ENTRY size_t dlmalloc_usable_size(void*);

} // namespace memory_dl

#if !PLY_DLL_IMPORTING
Heap_t Heap;
#endif // !PLY_DLL_IMPORTING

void Heap_t::zeroInit() {
    memset(static_cast<void*>(this), 0, sizeof(*this));
}

void* Heap_t::alloc(ureg size) {
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    return memory_dl::dlmalloc((size_t) size, &this->mstate);
}

void* Heap_t::realloc(void* ptr, ureg newSize) {
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    return memory_dl::dlrealloc(ptr, (size_t) newSize, &this->mstate);
}

void Heap_t::free(void* ptr) {
    if (!ptr)
        return;
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    return memory_dl::dlfree(ptr, &this->mstate);
}

void* Heap_t::allocAligned(ureg size, ureg alignment) {
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    return memory_dl::dlmemalign((size_t) alignment, (size_t) size, &this->mstate);
}

void Heap_t::freeAligned(void* ptr) {
    free(ptr);
}

HeapStats Heap_t::getStats() {
    HeapStats stats;
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    memory_dl::dlmalloc_stats(&this->mstate, stats);
    return stats;
}

#if PLY_DLMALLOC_FAST_STATS
ureg Heap_t::getInUseBytes() const {
    return this->mstate.inUseBytes;
}
#endif

ureg Heap_t::getSize(void* ptr) {
    // No need to lock
    return memory_dl::dlmalloc_usable_size(ptr);
}

} // namespace ply

//---------------------------------------------------------------------------
// Override operators new/delete
// C++ allows us to replace global operators new/delete with our own thanks to
// weak linking.
// Note: We need to statically link these global operators even when Plywood is
// imported across DLL boundaries (ie. even when PLY_DLL_IMPORTING=1).
//---------------------------------------------------------------------------
void* operator new(std::size_t size) {
    return ply::Heap.alloc(size);
}

void* operator new[](std::size_t size) {
    return ply::Heap.alloc(size);
}

void operator delete(void* ptr) noexcept {
    ply::Heap.free(ptr);
}

void operator delete[](void* ptr) noexcept {
    ply::Heap.free(ptr);
}

#if PLY_WITH_EXCEPTIONS
void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return ply::Heap.alloc(size);
}

void* operator new[](std::size_t size, std::nothrow_t const&) noexcept {
    return ply::Heap.alloc(size);
}

void operator delete(void* ptr, std::nothrow_t const&) noexcept {
    ply::Heap.free(ptr);
}

void operator delete[](void* ptr, std::nothrow_t const&) noexcept {
    ply::Heap.free(ptr);
}
#endif // PLY_WITH_EXCEPTIONS
