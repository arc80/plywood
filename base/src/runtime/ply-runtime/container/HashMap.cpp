/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <string.h>

namespace ply {
namespace impl {

//------------------------------------------------------------------
// HashMap
//------------------------------------------------------------------
PLY_NO_INLINE HashMap::HashMap(HashMap&& other) {
    m_cellGroups = other.m_cellGroups;
    m_sizeMask = other.m_sizeMask;
    m_population = other.m_population;
    other.m_cellGroups = nullptr;
    other.m_sizeMask = 0;
    other.m_population = 0;
}

PLY_NO_INLINE HashMap::HashMap(const Callbacks* cb, u32 initial_size)
    : m_cellGroups(create_table(cb, initial_size)),
      m_sizeMask(safe_demote<u32>(initial_size - 1)), m_population{0} {
}

PLY_NO_INLINE void HashMap::move_assign(const Callbacks* cb, HashMap&& other) {
    if (m_cellGroups) {
        destroy_table(cb, m_cellGroups, m_sizeMask + 1);
    }
    new (this) impl::HashMap{std::move((impl::HashMap&) other)};
}

PLY_NO_INLINE void HashMap::clear(const Callbacks* cb) {
    if (m_cellGroups) {
        impl::HashMap::destroy_table(cb, m_cellGroups, m_sizeMask + 1);
    }
    m_cellGroups = nullptr;
    m_sizeMask = 0;
    m_population = 0;
}

PLY_NO_INLINE HashMap::CellGroup* HashMap::create_table(const Callbacks* cb, u32 size) {
    PLY_ASSERT(size >= 4 && is_power_of2(size));
    u32 cell_group_size = sizeof(CellGroup) + cb->item_size * 4;
    u32 alloc_size = cell_group_size * (size >> 2);
    CellGroup* cell_groups = (CellGroup*) Heap.alloc(alloc_size);
    for (u32 i = 0; i < (size >> 2); i++) {
        u32* cell = (u32*) PLY_PTR_OFFSET(cell_groups, cell_group_size * i);
        *cell = (u32) -1;
        memset(cell + 1, 0, cell_group_size - 4);
    }
    return cell_groups;
}

PLY_NO_INLINE void HashMap::destroy_table(const Callbacks* cb, CellGroup* cell_groups,
                                          u32 size) {
    PLY_ASSERT(cell_groups);
    if (!cb->is_trivially_destructible) {
        u32 cell_group_size = sizeof(CellGroup) + cb->item_size * 4;
        for (u32 i = 0; i < size; i++) {
            CellGroup* group =
                (CellGroup*) PLY_PTR_OFFSET(cell_groups, cell_group_size * (i >> 2));
            if (group->next_delta[i & 3] != EmptySlot) {
                cb->destruct(PLY_PTR_OFFSET(group + 1, cb->item_size * (i & 3)));
            }
        }
    }
    Heap.free(cell_groups);
}

PLY_NO_INLINE void HashMap::migrate_to_new_table(const Callbacks* cb) {
    u32 desired_size = max(InitialSize, round_up_power_of2(u32(m_population * 2)));
    CellGroup* src_cell_groups = m_cellGroups;
    u32 src_size = m_sizeMask + 1;
    m_cellGroups = create_table(cb, desired_size);
    m_sizeMask = desired_size - 1;
    u32 cell_group_size = sizeof(CellGroup) + cb->item_size * 4;
    for (u32 src_idx = 0; src_idx < src_size; src_idx++) {
        CellGroup* src_group = (CellGroup*) PLY_PTR_OFFSET(
            src_cell_groups, cell_group_size * (src_idx >> 2));
        if (src_group->next_delta[src_idx & 3] != EmptySlot) {
            void* dst_item =
                insert_for_migration(cb->item_size, src_group->hashes[src_idx & 3]);
            void* src_item =
                PLY_PTR_OFFSET(src_group + 1, cb->item_size * (src_idx & 3));
            if (dst_item) {
                cb->move_construct(dst_item, src_item);
            }
            cb->destruct(src_item);
        }
    }
    Heap.free(src_cell_groups);
}

PLY_NO_INLINE HashMap::FindResult HashMap::find_next(FindInfo* info,
                                                     const Callbacks* cb,
                                                     const void* key,
                                                     const void* context) const {
    PLY_ASSERT(m_cellGroups);
    PLY_ASSERT(info->item_slot);
    u32 cell_group_size = sizeof(CellGroup) + cb->item_size * 4;
    CellGroup* group = (CellGroup*) PLY_PTR_OFFSET(
        m_cellGroups, cell_group_size * ((info->idx & m_sizeMask) >> 2));
    PLY_ASSERT(info->item_slot ==
               PLY_PTR_OFFSET(group + 1, cb->item_size * (info->idx & 3)));
    u32 hash = group->hashes[info->idx & 3];

    // Follow probe chain for our bucket.
    info->prev_link =
        (info->prev_link ? group->first_delta : group->next_delta) + (info->idx & 3);
    u8 delta = *info->prev_link;
    while (delta) {
        info->idx += delta;
        group = m_cellGroups + ((info->idx & m_sizeMask) >> 2);
        if (group->hashes[info->idx & 3] == hash) {
            void* item = PLY_PTR_OFFSET(group + 1, cb->item_size * (info->idx & 3));
            if (cb->match(item, key, context)) {
                info->item_slot = item;
                return FindResult::Found;
            }
        }
        info->prev_link = group->next_delta + (info->idx & 3);
        delta = *info->prev_link;
        PLY_ASSERT(delta != EmptySlot);
    }

    // End of probe chain, not found
    info->item_slot = nullptr;
    return FindResult::NotFound;
}

PLY_NO_INLINE HashMap::FindResult
HashMap::insert_or_find(FindInfo* info, const Callbacks* cb, const void* key,
                        const void* context, u32 flags) {
    PLY_ASSERT((context != nullptr) == cb->requires_context);
    PLY_ASSERT(m_cellGroups);
    u32 hash = cb->hash(key);
    info->idx = hash;
    info->prev_link = nullptr;

    // Check hashed cell first, though it may not even belong to the bucket.
    u32 cell_group_size = sizeof(CellGroup) + cb->item_size * 4;
    CellGroup* group = (CellGroup*) PLY_PTR_OFFSET(
        m_cellGroups, cell_group_size * ((info->idx & m_sizeMask) >> 2));
    if (((flags & AllowInsert) != 0) &&
        (group->next_delta[info->idx & 3] == EmptySlot) &&
        (group->first_delta[info->idx & 3] == 0)) {
        // Reserve the first cell.
        group->next_delta[info->idx & 3] = 0;
        group->hashes[info->idx & 3] = hash;
        info->item_slot = PLY_PTR_OFFSET(group + 1, cb->item_size * (info->idx & 3));
        cb->construct(info->item_slot, key);
        return FindResult::InsertedNew;
    }
    if (((flags & AllowFind) != 0) && (group->next_delta[info->idx & 3] != EmptySlot) &&
        (group->hashes[info->idx & 3] == hash)) {
        void* item = PLY_PTR_OFFSET(group + 1, cb->item_size * (info->idx & 3));
        if (cb->match(item, key, context)) {
            info->item_slot = item;
            return FindResult::Found;
        }
    }

    // Follow probe chain for our bucket.
    info->prev_link = group->first_delta + (info->idx & 3);
    u8 delta = *info->prev_link;
    while (delta) {
        info->idx += delta;
        group = (CellGroup*) PLY_PTR_OFFSET(
            m_cellGroups, cell_group_size * ((info->idx & m_sizeMask) >> 2));
        PLY_ASSERT(group->next_delta[info->idx & 3] != EmptySlot);
        if (((flags & AllowFind) != 0) && (group->hashes[info->idx & 3] == hash)) {
            void* item = PLY_PTR_OFFSET(group + 1, cb->item_size * (info->idx & 3));
            if (cb->match(item, key, context)) {
                info->item_slot = item;
                return FindResult::Found; // Item found in table
            }
        }
        info->prev_link = group->next_delta + (info->idx & 3);
        delta = *info->prev_link;
        PLY_ASSERT(delta != EmptySlot);
    }

    if ((flags & AllowInsert) == 0) {
        // End of probe chain, not found
        info->item_slot = nullptr;
        return FindResult::NotFound;
    }

    // Reached the end of the link chain for this bucket.
    // Switch to linear probing to find a free cell.
    u32 prev_link_idx = info->idx;
    PLY_ASSERT(info->idx - hash <= m_sizeMask);
    u32 linear_probes_remaining =
        min(m_sizeMask - (info->idx - hash), LinearSearchLimit);
    while (linear_probes_remaining-- > 0) {
        info->idx++;
        group = (CellGroup*) PLY_PTR_OFFSET(
            m_cellGroups, cell_group_size * ((info->idx & m_sizeMask) >> 2));
        if (group->next_delta[info->idx & 3] == EmptySlot) {
            // It's an empty cell. Insert it here.
            group->next_delta[info->idx & 3] = 0;
            group->hashes[info->idx & 3] = hash;
            // Link it to previous cell in the same bucket.
            *info->prev_link = info->idx - prev_link_idx;
            info->item_slot =
                PLY_PTR_OFFSET(group + 1, cb->item_size * (info->idx & 3));
            cb->construct(info->item_slot, key);
            return FindResult::InsertedNew;
        }
        // In a single-threaded HashMap, it's impossible for a matching hash to appear
        // outside the probe chain.
        PLY_ASSERT(group->hashes[info->idx & 3] != hash);
        // Continue linear search...
    }

    // Table is too full to insert.
    info->idx++;
    return FindResult::Overflow;
}

PLY_NO_INLINE void* HashMap::insert_for_migration(u32 item_size, u32 hash) {
    PLY_ASSERT(m_cellGroups);
    u32 idx = hash;

    // Check hashed cell first, though it may not even belong to the bucket.
    u32 cell_group_size = sizeof(CellGroup) + item_size * 4;
    CellGroup* group = (CellGroup*) PLY_PTR_OFFSET(
        m_cellGroups, cell_group_size * ((idx & m_sizeMask) >> 2));
    if (group->next_delta[idx & 3] == EmptySlot) {
        PLY_ASSERT(group->first_delta[idx & 3] == 0);
        // Reserve the first cell.
        group->next_delta[idx & 3] = 0;
        group->hashes[idx & 3] = hash;
        return PLY_PTR_OFFSET(group + 1, item_size * (idx & 3));
    }

    // Follow probe chain for our bucket.
    u8* prev_link = group->first_delta + (idx & 3);
    u8 delta = *prev_link;
    while (delta) {
        idx += delta;
        group = (CellGroup*) PLY_PTR_OFFSET(
            m_cellGroups, cell_group_size * ((idx & m_sizeMask) >> 2));
        prev_link = group->next_delta + (idx & 3);
        delta = *prev_link;
        PLY_ASSERT(delta !=
                   EmptySlot); // Shouldn't be marked unused since a link pointed here
    }

    // Reached the end of the link chain for this bucket.
    // Switch to linear probing to find a free cell.
    u32 prev_link_idx = idx;
    u32 linear_probes_remaining = min(m_sizeMask, LinearSearchLimit);
    while (linear_probes_remaining-- > 0) {
        idx++;
        group = (CellGroup*) PLY_PTR_OFFSET(
            m_cellGroups, cell_group_size * ((idx & m_sizeMask) >> 2));
        if (group->next_delta[idx & 3] == EmptySlot) {
            // It's an empty cell. Insert it here.
            group->next_delta[idx & 3] = 0;
            group->hashes[idx & 3] = hash;
            // Link it to previous cell in the same bucket.
            *prev_link = idx - prev_link_idx;
            void* item_slot = PLY_PTR_OFFSET(group + 1, item_size * (idx & 3));
            return item_slot;
        }
        // In a single-threaded HashMap, it's impossible for a matching hash to appear
        // outside the probe chain.
        PLY_ASSERT(group->hashes[idx & 3] != hash);
        // Continue linear search...
    }

    // There is a range of sequential cells that is too full to insert.
    // This may happen if the hash function doesn't distribute keys uniformly enough.
    // Shouldn't happen otherwise!
    PLY_ASSERT(0);
    return nullptr;
}

PLY_NO_INLINE void HashMap::erase(FindInfo* info, const Callbacks* cb,
                                  u8*& link_to_adjust) {
    PLY_ASSERT(info->item_slot);
    u32 idx = info->idx;
    u8* prev_link = info->prev_link;
    u32 cell_group_size = sizeof(CellGroup) + cb->item_size * 4;
    if (prev_link) {
        // If prev_link is a valid pointer, its value must be non-zero and it must act
        // as a valid link:
        PLY_ASSERT(*prev_link != 0);
        u32 prev_idx = idx - *prev_link;
        CellGroup* prev_group = (CellGroup*) PLY_PTR_OFFSET(
            m_cellGroups, cell_group_size * ((prev_idx & m_sizeMask) >> 2));
        PLY_ASSERT((prev_link == prev_group->first_delta + (prev_idx & 3)) ||
                   (prev_link == prev_group->next_delta + (prev_idx & 3)));
        PLY_UNUSED(prev_group);
    }
    CellGroup* cur_group = (CellGroup*) PLY_PTR_OFFSET(
        m_cellGroups, cell_group_size * ((idx & m_sizeMask) >> 2));
    for (;;) {
        u8* cur_link = cur_group->next_delta + (idx & 3);
        if (*cur_link == 0) {
            // Erasing the last item in a chain
            cb->destruct(PLY_PTR_OFFSET(cur_group + 1, cb->item_size * (idx & 3)));
            *cur_link = EmptySlot;
            if (prev_link) {
                *prev_link = 0;
            }
            break;
        }
        u32 next_idx = idx + *cur_link;
        CellGroup* next_group = m_cellGroups + ((next_idx & m_sizeMask) >> 2);
        if (prev_link) {
            u32 prev_idx = idx - *prev_link;
            if (next_idx - prev_idx < 256) {
                // Just redirect the previous link
                cb->destruct(PLY_PTR_OFFSET(cur_group + 1, cb->item_size * (idx & 3)));
                *cur_link = EmptySlot;
                *prev_link = next_idx - prev_idx;
                if (link_to_adjust == cur_link) {
                    link_to_adjust = cur_link;
                }
                break;
            }
        }
        // Shuffle the next item into the current slot
        cur_group->hashes[idx & 3] = next_group->hashes[next_idx & 3];
        cb->move_assign(PLY_PTR_OFFSET(cur_group + 1, cb->item_size * (idx & 3)),
                        PLY_PTR_OFFSET(next_group + 1, cb->item_size * (next_idx & 3)));
        // Try to erase the next slot
        idx = next_idx;
        prev_link = cur_link;
        cur_group = next_group;
    }
    PLY_ASSERT(m_population > 0);
    m_population--;
}

//------------------------------------------------------------------
// HashMap::Cursor
//------------------------------------------------------------------
PLY_NO_INLINE void HashMap::Cursor::construct_find_with_insert(const Callbacks* cb,
                                                               HashMap* map,
                                                               const void* key,
                                                               const void* context,
                                                               u32 flags) {
    m_map = map;
    for (;;) {
        m_findResult = m_map->insert_or_find(&m_findInfo, cb, key, context, flags);
        if (m_findResult != impl::HashMap::FindResult::Overflow) {
            if (m_findResult == impl::HashMap::FindResult::InsertedNew) {
                m_map->m_population++;
            }
            return;
        }
        // Insert overflow. Migrate and try again.
        reinterpret_cast<impl::HashMap*>(m_map)->migrate_to_new_table(cb);
    }
}

} // namespace impl
} // namespace ply
