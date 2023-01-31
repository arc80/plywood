/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime.h>

namespace ply {
namespace impl {

PLY_NO_INLINE void BaseArray::alloc(u32 numItems, u32 itemSize) {
    m_allocated = roundUpPowerOf2(numItems);
    m_items = Heap.alloc(ureg(m_allocated) * itemSize);
    m_numItems = numItems;
}

PLY_NO_INLINE void BaseArray::realloc(u32 numItems, u32 itemSize) {
    m_allocated = roundUpPowerOf2(numItems);
    m_items = Heap.realloc(m_items, ureg(m_allocated) * itemSize);
    m_numItems = numItems;
}

PLY_NO_INLINE void BaseArray::free() {
    Heap.free(m_items);
}

PLY_NO_INLINE void BaseArray::reserve(u32 numItems, u32 itemSize) {
    if (numItems > m_allocated) {
        m_allocated = roundUpPowerOf2(numItems); // FIXME: Generalize to other resize strategies?
        m_items = Heap.realloc(m_items, ureg(m_allocated) * itemSize);
    }
}

PLY_NO_INLINE void BaseArray::reserveIncrement(u32 itemSize) {
    reserve(m_numItems + 1, itemSize);
}

PLY_NO_INLINE void BaseArray::truncate(u32 itemSize) {
    m_allocated = m_numItems;
    m_items = Heap.realloc(m_items, ureg(m_allocated) * itemSize);
}

} // namespace impl
} // namespace ply
