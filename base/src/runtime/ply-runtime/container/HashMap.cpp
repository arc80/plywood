/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/HashMap.h>
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

PLY_NO_INLINE HashMap::HashMap(const Callbacks* cb, u32 initialSize)
    : m_cellGroups(createTable(cb, initialSize)),
      m_sizeMask(safeDemote<u32>(initialSize - 1)), m_population{0} {
}

PLY_NO_INLINE void HashMap::moveAssign(const Callbacks* cb, HashMap&& other) {
    if (m_cellGroups) {
        destroyTable(cb, m_cellGroups, m_sizeMask + 1);
    }
    new (this) impl::HashMap{std::move((impl::HashMap&) other)};
}

PLY_NO_INLINE void HashMap::clear(const Callbacks* cb) {
    if (m_cellGroups) {
        impl::HashMap::destroyTable(cb, m_cellGroups, m_sizeMask + 1);
    }
    m_cellGroups = nullptr;
    m_sizeMask = 0;
    m_population = 0;
}

PLY_NO_INLINE HashMap::CellGroup* HashMap::createTable(const Callbacks* cb, u32 size) {
    PLY_ASSERT(size >= 4 && isPowerOf2(size));
    u32 cellGroupSize = sizeof(CellGroup) + cb->itemSize * 4;
    u32 allocSize = cellGroupSize * (size >> 2);
    CellGroup* cellGroups = (CellGroup*) Heap.alloc(allocSize);
    for (u32 i = 0; i < (size >> 2); i++) {
        u32* cell = (u32*) PLY_PTR_OFFSET(cellGroups, cellGroupSize * i);
        *cell = (u32) -1;
        memset(cell + 1, 0, cellGroupSize - 4);
    }
    return cellGroups;
}

PLY_NO_INLINE void HashMap::destroyTable(const Callbacks* cb, CellGroup* cellGroups, u32 size) {
    PLY_ASSERT(cellGroups);
    if (!cb->isTriviallyDestructible) {
        u32 cellGroupSize = sizeof(CellGroup) + cb->itemSize * 4;
        for (u32 i = 0; i < size; i++) {
            CellGroup* group = (CellGroup*) PLY_PTR_OFFSET(cellGroups, cellGroupSize * (i >> 2));
            if (group->nextDelta[i & 3] != EmptySlot) {
                cb->destruct(PLY_PTR_OFFSET(group + 1, cb->itemSize * (i & 3)));
            }
        }
    }
    Heap.free(cellGroups);
}

PLY_NO_INLINE void HashMap::migrateToNewTable(const Callbacks* cb) {
    u32 desiredSize = max(InitialSize, roundUpPowerOf2(u32(m_population * 2)));
    CellGroup* srcCellGroups = m_cellGroups;
    u32 srcSize = m_sizeMask + 1;
    m_cellGroups = createTable(cb, desiredSize);
    m_sizeMask = desiredSize - 1;
    u32 cellGroupSize = sizeof(CellGroup) + cb->itemSize * 4;
    for (u32 srcIdx = 0; srcIdx < srcSize; srcIdx++) {
        CellGroup* srcGroup =
            (CellGroup*) PLY_PTR_OFFSET(srcCellGroups, cellGroupSize * (srcIdx >> 2));
        if (srcGroup->nextDelta[srcIdx & 3] != EmptySlot) {
            void* dstItem = insertForMigration(cb->itemSize, srcGroup->hashes[srcIdx & 3]);
            void* srcItem = PLY_PTR_OFFSET(srcGroup + 1, cb->itemSize * (srcIdx & 3));
            if (dstItem) {
                cb->moveConstruct(dstItem, srcItem);
            }
            cb->destruct(srcItem);
        }
    }
    Heap.free(srcCellGroups);
}

PLY_NO_INLINE HashMap::FindResult HashMap::findNext(FindInfo* info, const Callbacks* cb,
                                                    const void* key, const void* context) const {
    PLY_ASSERT(m_cellGroups);
    PLY_ASSERT(info->itemSlot);
    u32 cellGroupSize = sizeof(CellGroup) + cb->itemSize * 4;
    CellGroup* group =
        (CellGroup*) PLY_PTR_OFFSET(m_cellGroups, cellGroupSize * ((info->idx & m_sizeMask) >> 2));
    PLY_ASSERT(info->itemSlot == PLY_PTR_OFFSET(group + 1, cb->itemSize * (info->idx & 3)));
    u32 hash = group->hashes[info->idx & 3];

    // Follow probe chain for our bucket.
    info->prevLink = (info->prevLink ? group->firstDelta : group->nextDelta) + (info->idx & 3);
    u8 delta = *info->prevLink;
    while (delta) {
        info->idx += delta;
        group = m_cellGroups + ((info->idx & m_sizeMask) >> 2);
        if (group->hashes[info->idx & 3] == hash) {
            void* item = PLY_PTR_OFFSET(group + 1, cb->itemSize * (info->idx & 3));
            if (cb->match(item, key, context)) {
                info->itemSlot = item;
                return FindResult::Found;
            }
        }
        info->prevLink = group->nextDelta + (info->idx & 3);
        delta = *info->prevLink;
        PLY_ASSERT(delta != EmptySlot);
    }

    // End of probe chain, not found
    info->itemSlot = nullptr;
    return FindResult::NotFound;
}

PLY_NO_INLINE HashMap::FindResult HashMap::insertOrFind(FindInfo* info, const Callbacks* cb,
                                                        const void* key, const void* context,
                                                        u32 flags) {
    PLY_ASSERT((context != nullptr) == cb->requiresContext);
    PLY_ASSERT(m_cellGroups);
    u32 hash = cb->hash(key);
    info->idx = hash;
    info->prevLink = nullptr;

    // Check hashed cell first, though it may not even belong to the bucket.
    u32 cellGroupSize = sizeof(CellGroup) + cb->itemSize * 4;
    CellGroup* group =
        (CellGroup*) PLY_PTR_OFFSET(m_cellGroups, cellGroupSize * ((info->idx & m_sizeMask) >> 2));
    if (((flags & AllowInsert) != 0) && (group->nextDelta[info->idx & 3] == EmptySlot) &&
        (group->firstDelta[info->idx & 3] == 0)) {
        // Reserve the first cell.
        group->nextDelta[info->idx & 3] = 0;
        group->hashes[info->idx & 3] = hash;
        info->itemSlot = PLY_PTR_OFFSET(group + 1, cb->itemSize * (info->idx & 3));
        cb->construct(info->itemSlot, key);
        return FindResult::InsertedNew;
    }
    if (((flags & AllowFind) != 0) && (group->nextDelta[info->idx & 3] != EmptySlot) &&
        (group->hashes[info->idx & 3] == hash)) {
        void* item = PLY_PTR_OFFSET(group + 1, cb->itemSize * (info->idx & 3));
        if (cb->match(item, key, context)) {
            info->itemSlot = item;
            return FindResult::Found;
        }
    }

    // Follow probe chain for our bucket.
    info->prevLink = group->firstDelta + (info->idx & 3);
    u8 delta = *info->prevLink;
    while (delta) {
        info->idx += delta;
        group = (CellGroup*) PLY_PTR_OFFSET(m_cellGroups,
                                            cellGroupSize * ((info->idx & m_sizeMask) >> 2));
        PLY_ASSERT(group->nextDelta[info->idx & 3] != EmptySlot);
        if (((flags & AllowFind) != 0) && (group->hashes[info->idx & 3] == hash)) {
            void* item = PLY_PTR_OFFSET(group + 1, cb->itemSize * (info->idx & 3));
            if (cb->match(item, key, context)) {
                info->itemSlot = item;
                return FindResult::Found; // Item found in table
            }
        }
        info->prevLink = group->nextDelta + (info->idx & 3);
        delta = *info->prevLink;
        PLY_ASSERT(delta != EmptySlot);
    }

    if ((flags & AllowInsert) == 0) {
        // End of probe chain, not found
        info->itemSlot = nullptr;
        return FindResult::NotFound;
    }

    // Reached the end of the link chain for this bucket.
    // Switch to linear probing to find a free cell.
    u32 prevLinkIdx = info->idx;
    PLY_ASSERT(info->idx - hash <= m_sizeMask);
    u32 linearProbesRemaining = min(m_sizeMask - (info->idx - hash), LinearSearchLimit);
    while (linearProbesRemaining-- > 0) {
        info->idx++;
        group = (CellGroup*) PLY_PTR_OFFSET(m_cellGroups,
                                            cellGroupSize * ((info->idx & m_sizeMask) >> 2));
        if (group->nextDelta[info->idx & 3] == EmptySlot) {
            // It's an empty cell. Insert it here.
            group->nextDelta[info->idx & 3] = 0;
            group->hashes[info->idx & 3] = hash;
            // Link it to previous cell in the same bucket.
            *info->prevLink = info->idx - prevLinkIdx;
            info->itemSlot = PLY_PTR_OFFSET(group + 1, cb->itemSize * (info->idx & 3));
            cb->construct(info->itemSlot, key);
            return FindResult::InsertedNew;
        }
        // In a single-threaded HashMap, it's impossible for a matching hash to appear outside
        // the probe chain.
        PLY_ASSERT(group->hashes[info->idx & 3] != hash);
        // Continue linear search...
    }

    // Table is too full to insert.
    info->idx++;
    return FindResult::Overflow;
}

PLY_NO_INLINE void* HashMap::insertForMigration(u32 itemSize, u32 hash) {
    PLY_ASSERT(m_cellGroups);
    u32 idx = hash;

    // Check hashed cell first, though it may not even belong to the bucket.
    u32 cellGroupSize = sizeof(CellGroup) + itemSize * 4;
    CellGroup* group =
        (CellGroup*) PLY_PTR_OFFSET(m_cellGroups, cellGroupSize * ((idx & m_sizeMask) >> 2));
    if (group->nextDelta[idx & 3] == EmptySlot) {
        PLY_ASSERT(group->firstDelta[idx & 3] == 0);
        // Reserve the first cell.
        group->nextDelta[idx & 3] = 0;
        group->hashes[idx & 3] = hash;
        return PLY_PTR_OFFSET(group + 1, itemSize * (idx & 3));
    }

    // Follow probe chain for our bucket.
    u8* prevLink = group->firstDelta + (idx & 3);
    u8 delta = *prevLink;
    while (delta) {
        idx += delta;
        group =
            (CellGroup*) PLY_PTR_OFFSET(m_cellGroups, cellGroupSize * ((idx & m_sizeMask) >> 2));
        prevLink = group->nextDelta + (idx & 3);
        delta = *prevLink;
        PLY_ASSERT(delta != EmptySlot); // Shouldn't be marked unused since a link pointed here
    }

    // Reached the end of the link chain for this bucket.
    // Switch to linear probing to find a free cell.
    u32 prevLinkIdx = idx;
    u32 linearProbesRemaining = min(m_sizeMask, LinearSearchLimit);
    while (linearProbesRemaining-- > 0) {
        idx++;
        group =
            (CellGroup*) PLY_PTR_OFFSET(m_cellGroups, cellGroupSize * ((idx & m_sizeMask) >> 2));
        if (group->nextDelta[idx & 3] == EmptySlot) {
            // It's an empty cell. Insert it here.
            group->nextDelta[idx & 3] = 0;
            group->hashes[idx & 3] = hash;
            // Link it to previous cell in the same bucket.
            *prevLink = idx - prevLinkIdx;
            void* itemSlot = PLY_PTR_OFFSET(group + 1, itemSize * (idx & 3));
            return itemSlot;
        }
        // In a single-threaded HashMap, it's impossible for a matching hash to appear outside
        // the probe chain.
        PLY_ASSERT(group->hashes[idx & 3] != hash);
        // Continue linear search...
    }

    // There is a range of sequential cells that is too full to insert.
    // This may happen if the hash function doesn't distribute keys uniformly enough.
    // Shouldn't happen otherwise!
    PLY_ASSERT(0);
    return nullptr;
}

PLY_NO_INLINE void HashMap::erase(FindInfo* info, const Callbacks* cb, u8*& linkToAdjust) {
    PLY_ASSERT(info->itemSlot);
    u32 idx = info->idx;
    u8* prevLink = info->prevLink;
    u32 cellGroupSize = sizeof(CellGroup) + cb->itemSize * 4;
    if (prevLink) {
        // If prevLink is a valid pointer, its value must be non-zero and it must act as a
        // valid link:
        PLY_ASSERT(*prevLink != 0);
        u32 prevIdx = idx - *prevLink;
        CellGroup* prevGroup = (CellGroup*) PLY_PTR_OFFSET(
            m_cellGroups, cellGroupSize * ((prevIdx & m_sizeMask) >> 2));
        PLY_ASSERT((prevLink == prevGroup->firstDelta + (prevIdx & 3)) ||
                   (prevLink == prevGroup->nextDelta + (prevIdx & 3)));
        PLY_UNUSED(prevGroup);
    }
    CellGroup* curGroup =
        (CellGroup*) PLY_PTR_OFFSET(m_cellGroups, cellGroupSize * ((idx & m_sizeMask) >> 2));
    for (;;) {
        u8* curLink = curGroup->nextDelta + (idx & 3);
        if (*curLink == 0) {
            // Erasing the last item in a chain
            cb->destruct(PLY_PTR_OFFSET(curGroup + 1, cb->itemSize * (idx & 3)));
            *curLink = EmptySlot;
            if (prevLink) {
                *prevLink = 0;
            }
            break;
        }
        u32 nextIdx = idx + *curLink;
        CellGroup* nextGroup = m_cellGroups + ((nextIdx & m_sizeMask) >> 2);
        if (prevLink) {
            u32 prevIdx = idx - *prevLink;
            if (nextIdx - prevIdx < 256) {
                // Just redirect the previous link
                cb->destruct(PLY_PTR_OFFSET(curGroup + 1, cb->itemSize * (idx & 3)));
                *curLink = EmptySlot;
                *prevLink = nextIdx - prevIdx;
                if (linkToAdjust == curLink) {
                    linkToAdjust = curLink;
                }
                break;
            }
        }
        // Shuffle the next item into the current slot
        curGroup->hashes[idx & 3] = nextGroup->hashes[nextIdx & 3];
        cb->moveAssign(PLY_PTR_OFFSET(curGroup + 1, cb->itemSize * (idx & 3)),
                       PLY_PTR_OFFSET(nextGroup + 1, cb->itemSize * (nextIdx & 3)));
        // Try to erase the next slot
        idx = nextIdx;
        prevLink = curLink;
        curGroup = nextGroup;
    }
    PLY_ASSERT(m_population > 0);
    m_population--;
}

//------------------------------------------------------------------
// HashMap::Cursor
//------------------------------------------------------------------
PLY_NO_INLINE void HashMap::Cursor::constructFindWithInsert(const Callbacks* cb, HashMap* map,
                                                            const void* key, const void* context,
                                                            u32 flags) {
    m_map = map;
    for (;;) {
        m_findResult = m_map->insertOrFind(&m_findInfo, cb, key, context, flags);
        if (m_findResult != impl::HashMap::FindResult::Overflow) {
            if (m_findResult == impl::HashMap::FindResult::InsertedNew) {
                m_map->m_population++;
            }
            return;
        }
        // Insert overflow. Migrate and try again.
        reinterpret_cast<impl::HashMap*>(m_map)->migrateToNewTable(cb);
    }
}

} // namespace impl
} // namespace ply
