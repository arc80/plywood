/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {

template <typename T>
u32 new_hash(const T& val) {
    Hasher h;
    h << val;
    return h.result();
}
template <>
u32 new_hash<u32>(const u32& val) {
    return avalanche(val);
}
template <>
u32 new_hash<u64>(const u64& val) {
    return (u32) avalanche(val);
}

template <typename T, typename... Args>
void construct(T* obj, Args&&... args) {
    new (obj) T{std::forward<Args>(args)...};
}

template <typename Key>
void map_reindex(BaseMap* map, u32 num_indices, const MapTypeInfo* ti) {
    // Initialize all indices to -1.
    map->indices.resize(num_indices);
    map->indices.truncate();
    for (s32& i : map->indices) {
        i = -1;
    }

    // Rebuild indices.
    PLY_ASSERT(is_power_of2(map->indices.num_items()));
    u32 mask = map->indices.num_items() - 1;
    for (u32 item_idx = 0; item_idx < map->items.num_items; item_idx++) {
        void* key = PLY_PTR_OFFSET(map->items.items, item_idx * ti->item_size);
        for (u32 idx = new_hash(*(Key*) key);; idx++) {
            if (map->indices[idx & mask] < 0) {
                map->indices[idx & mask] = item_idx;
                break;
            }
        }
    }
}

template void map_reindex<u32>(BaseMap*, u32, const MapTypeInfo*);
template void map_reindex<u64>(BaseMap*, u32, const MapTypeInfo*);
template void map_reindex<StringView>(BaseMap*, u32, const MapTypeInfo*);

u32 best_num_indices(u32 num_items) {
    if (num_items >= 8) {
        return round_up_power_of2(u32((u64{num_items} * 5) >> 2));
    }
    return (num_items < 4) ? 4 : 8;
}

template <typename Key>
void* map_operate(BaseMap* map, MapOperation op, View<Key> key, const MapTypeInfo* ti,
                  bool* was_found) {
    if (was_found) {
        *was_found = false;
    }
    if (op == M_Insert) {
        u32 min_indices = best_num_indices(map->items.num_items + 1);
        if (map->indices.num_items() < min_indices) {
            PLY_PUN_SCOPE
            map_reindex<View<Key>>(map, min_indices, ti);
        }
    } else if (!map->indices) {
        return nullptr;
    }

    PLY_ASSERT(is_power_of2(map->indices.num_items()));
    u32 mask = map->indices.num_items() - 1;
    for (u32 idx = new_hash(key);; idx++) {
        s32 item_idx = map->indices[idx & mask];
        if (item_idx >= 0) {
            void* item = PLY_PTR_OFFSET(map->items.items, item_idx * ti->item_size);
            if (*((Key*) item) == key) {
                // Found existing item.
                if (was_found) {
                    *was_found = true;
                }
                return PLY_PTR_OFFSET(item, ti->value_offset);
            }
        } else {
            if (op == M_Find) {
                return nullptr;
            } else {
                // Store item index.
                item_idx = safe_demote<s32>(map->items.num_items);
                map->indices[idx & mask] = item_idx;

                // Construct new item.
                if (map->items.num_items >= map->items.allocated) {
                    map->items.reserve_increment(ti->item_size);
                }
                map->items.num_items++;
                void* item = PLY_PTR_OFFSET(map->items.items, item_idx * ti->item_size);
                construct((Key*) item, key);
                void* value = PLY_PTR_OFFSET(item, ti->value_offset);
                ti->construct(value);
                return value;
            }
        }
    }
}

// Explicit instantiations.
template void* map_operate<u32>(BaseMap*, MapOperation, u32, const MapTypeInfo*, bool*);
template void* map_operate<u64>(BaseMap*, MapOperation, u64, const MapTypeInfo*, bool*);
template void* map_operate<String>(BaseMap*, MapOperation, StringView,
                                   const MapTypeInfo*, bool*);
template void* map_operate<StringView>(BaseMap*, MapOperation, StringView,
                                       const MapTypeInfo*, bool*);

} // namespace ply