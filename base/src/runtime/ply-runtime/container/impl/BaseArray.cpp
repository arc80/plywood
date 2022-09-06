/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/impl/BaseArray.h>
#include <ply-runtime/memory/Heap.h>

namespace ply {
namespace impl {

PLY_NO_INLINE void BaseArray::alloc(u32 numItems, u32 itemSize) {
    m_allocated = roundUpPowerOf2(numItems);
    m_items = PLY_HEAP.alloc(ureg(m_allocated) * itemSize);
    m_numItems = numItems;
}

PLY_NO_INLINE void BaseArray::realloc(u32 numItems, u32 itemSize) {
    m_allocated = roundUpPowerOf2(numItems);
    m_items = PLY_HEAP.realloc(m_items, ureg(m_allocated) * itemSize);
    m_numItems = numItems;
}

PLY_NO_INLINE void BaseArray::free() {
    PLY_HEAP.free(m_items);
}

PLY_NO_INLINE void BaseArray::reserve(u32 numItems, u32 itemSize) {
    if (numItems > m_allocated) {
        m_allocated = roundUpPowerOf2(numItems); // FIXME: Generalize to other resize strategies?
        m_items = PLY_HEAP.realloc(m_items, ureg(m_allocated) * itemSize);
    }
}

PLY_NO_INLINE void BaseArray::reserveIncrement(u32 itemSize) {
    reserve(m_numItems + 1, itemSize);
}

PLY_NO_INLINE void BaseArray::truncate(u32 itemSize) {
    m_allocated = m_numItems;
    m_items = PLY_HEAP.realloc(m_items, ureg(m_allocated) * itemSize);
}

} // namespace impl
} // namespace ply
