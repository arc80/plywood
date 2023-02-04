﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <algorithm>

namespace ply {

void BasePool::baseSortFreeList(u32 itemSize) const {
    PLY_ASSERT(itemSize >= sizeof(u32));
    if (m_sortedFreeList)
        return;
    if (m_firstFree == u32(-1))
        return;

    // There is at least one item in the freelist
    PLY_ASSERT(m_freeListSize > 0);
    u32* freeIndices = new u32[m_freeListSize];
    u32 numFreeIndices = 0;
    for (u32 freeIndex = m_firstFree; freeIndex != u32(-1);) {
        freeIndices[numFreeIndices] = freeIndex;
        freeIndex = *(u32*) PLY_PTR_OFFSET(m_items, freeIndex * itemSize);
        numFreeIndices++;
    }
    PLY_ASSERT(numFreeIndices == m_freeListSize);
    std::sort(freeIndices, freeIndices + numFreeIndices);
    u32 prevIndex = freeIndices[0];
    m_firstFree = prevIndex;
    for (u32 i = 1; i < numFreeIndices; i++) {
        u32 nextFree = freeIndices[i];
        *(u32*) PLY_PTR_OFFSET(m_items, prevIndex * itemSize) = nextFree;
        prevIndex = nextFree;
    }
    *(u32*) PLY_PTR_OFFSET(m_items, prevIndex * itemSize) = u32(-1);
    delete[] freeIndices;
    m_sortedFreeList = true;
}

} // namespace ply
