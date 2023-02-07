/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {
namespace memory_dl {

void* dlmalloc(size_t, mstate);
void dlfree(void*, mstate);
void* dlcalloc(size_t, size_t, mstate);
void* dlrealloc(void*, size_t, mstate);
void* dlrealloc_in_place(void*, size_t, mstate);
void* dlmemalign(size_t, size_t, mstate);
int dlposix_memalign(void**, size_t, size_t, mstate);
void* dlvalloc(size_t, mstate);
int dlmallopt(int, int, mstate);
size_t dlmalloc_footprint(mstate);
size_t dlmalloc_max_footprint(mstate);
size_t dlmalloc_footprint_limit(mstate);
size_t dlmalloc_set_footprint_limit(size_t bytes, mstate);
void** dlindependent_calloc(size_t, size_t, void**, mstate);
void** dlindependent_comalloc(size_t, size_t*, void**, mstate);
size_t dlbulk_free(void**, size_t n_elements, mstate);
void* dlpvalloc(size_t, mstate);
int dlmalloc_trim(size_t, mstate);
void dlmalloc_stats(mstate, HeapStats&);
size_t dlmalloc_usable_size(void*);

} // namespace memory_dl

#if !PLY_DLL_IMPORTING
Heap_t Heap;
#endif // !PLY_DLL_IMPORTING

void Heap_t::zero_init() {
    memset(static_cast<void*>(this), 0, sizeof(*this));
}

void* Heap_t::alloc(ureg size) {
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    return memory_dl::dlmalloc((size_t) size, &this->mstate);
}

void* Heap_t::realloc(void* ptr, ureg new_size) {
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    return memory_dl::dlrealloc(ptr, (size_t) new_size, &this->mstate);
}

void Heap_t::free(void* ptr) {
    if (!ptr)
        return;
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    return memory_dl::dlfree(ptr, &this->mstate);
}

void* Heap_t::alloc_aligned(ureg size, ureg alignment) {
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    return memory_dl::dlmemalign((size_t) alignment, (size_t) size, &this->mstate);
}

void Heap_t::free_aligned(void* ptr) {
    free(ptr);
}

HeapStats Heap_t::get_stats() {
    HeapStats stats;
    LockGuard<Mutex_LazyInit> guard(this->mutex);
    memory_dl::dlmalloc_stats(&this->mstate, stats);
    return stats;
}

#if PLY_DLMALLOC_FAST_STATS
ureg Heap_t::get_in_use_bytes() const {
    return this->mstate.in_use_bytes;
}
#endif

ureg Heap_t::get_size(void* ptr) {
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
