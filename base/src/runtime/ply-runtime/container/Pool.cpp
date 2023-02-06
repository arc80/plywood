/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <algorithm>

namespace ply {

void BasePool::base_sort_free_list(u32 item_size) const {
    PLY_ASSERT(item_size >= sizeof(u32));
    if (m_sortedFreeList)
        return;
    if (m_firstFree == u32(-1))
        return;

    // There is at least one item in the freelist
    PLY_ASSERT(m_freeListSize > 0);
    u32* free_indices = new u32[m_freeListSize];
    u32 num_free_indices = 0;
    for (u32 free_index = m_firstFree; free_index != u32(-1);) {
        free_indices[num_free_indices] = free_index;
        free_index = *(u32*) PLY_PTR_OFFSET(m_items, free_index * item_size);
        num_free_indices++;
    }
    PLY_ASSERT(num_free_indices == m_freeListSize);
    std::sort(free_indices, free_indices + num_free_indices);
    u32 prev_index = free_indices[0];
    m_firstFree = prev_index;
    for (u32 i = 1; i < num_free_indices; i++) {
        u32 next_free = free_indices[i];
        *(u32*) PLY_PTR_OFFSET(m_items, prev_index * item_size) = next_free;
        prev_index = next_free;
    }
    *(u32*) PLY_PTR_OFFSET(m_items, prev_index * item_size) = u32(-1);
    delete[] free_indices;
    m_sortedFreeList = true;
}

} // namespace ply
