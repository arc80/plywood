/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/memory/Heap.h>

#if !PLY_DLL_IMPORTING
PLY_IMPL_HEAP_TYPE PlyHeap;
#endif // !PLY_DLL_IMPORTING

//---------------------------------------------------------------------------
// Override operators new/delete
// C++ allows us to replace global operators new/delete with our own thanks to
// weak linking.
// Note: We need to statically link these global operators even when Plywood is
// imported across DLL boundaries (ie. even when PLY_DLL_IMPORTING=1).
//---------------------------------------------------------------------------
#if PLY_REPLACE_OPERATOR_NEW
void* operator new(std::size_t size) {
    return PLY_HEAP.alloc(size);
}

void* operator new[](std::size_t size) {
    return PLY_HEAP.alloc(size);
}

void operator delete(void* ptr) noexcept {
    PLY_HEAP.free(ptr);
}

void operator delete[](void* ptr) noexcept {
    PLY_HEAP.free(ptr);
}

#if PLY_WITH_EXCEPTIONS
void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return PLY_HEAP.alloc(size);
}

void* operator new[](std::size_t size, std::nothrow_t const&) noexcept {
    return PLY_HEAP.alloc(size);
}

void operator delete(void* ptr, std::nothrow_t const&) noexcept {
    PLY_HEAP.free(ptr);
}

void operator delete[](void* ptr, std::nothrow_t const&) noexcept {
    PLY_HEAP.free(ptr);
}
#endif // PLY_WITH_EXCEPTIONS
#endif // PLY_REPLACE_OPERATOR_NEW
