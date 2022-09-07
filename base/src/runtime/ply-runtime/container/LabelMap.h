/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/Label.h>
#include <ply-runtime/container/impl/BaseLabelMap.h>

namespace ply {

template <typename T>
struct LabelMapIterator;

template <typename T>
class LabelMap {
private:
    struct Cell {
        Label keys[4];
        T values[4];
    };

    Cell* cells;
    u32 population;
    u32 capacity; // Always a power of 2

    using Base = impl::BaseLabelMap;
    template <typename T>
    friend struct LabelMapIterator;

public:
    PLY_INLINE LabelMap() {
        PLY_PUN_SCOPE
        construct((Base*) this, &Base::typeInfo<T>);
    }

    PLY_INLINE ~LabelMap() {
        PLY_PUN_SCOPE
        destruct((Base*) this, &Base::typeInfo<T>);
    }

    PLY_INLINE u32 numItems() const {
        return this->population;
    }

    PLY_INLINE T* find(Label key) {
        PLY_PUN_SCOPE
        T* value;
        operate((Base*) this, Base::Find, key, &Base::typeInfo<T>, (void**) &value);
        return value;
    }

    PLY_INLINE const T* find(Label key) const {
        PLY_PUN_SCOPE
        const T* value;
        operate((Base*) this, Base::Find, key, &Base::typeInfo<T>, &value);
        return value;
    }

    PLY_INLINE bool insertOrFind(Label key, T** value) {
        PLY_PUN_SCOPE
        return operate((Base*) this, Base::Insert, key, &Base::typeInfo<T>, (void**) value);
    }

    struct Item {
        Label key;
        T& value;
    };
    LabelMapIterator<T> begin();
    LabelMapIterator<T> end();
    LabelMapIterator<const T> begin() const;
    LabelMapIterator<const T> end() const;
};

template <typename T>
struct LabelMapIterator {
    using Map = LabelMap<T>;
    using Cell = typename Map::Cell;
    using Item = typename Map::Item;

    Map* map;
    u32 index;

    PLY_INLINE void operator++() {
        u32 mask = this->map->capacity - 1;
        while (true) {
            this->index++;
            PLY_ASSERT(this->index <= this->map->capacity);
            if (this->index >= this->map->capacity)
                break;
            Cell* cell = this->map->cells + ((this->index & mask) >> 2);
            if (cell->keys[this->index & 3])
                break;
        }
    }

    PLY_INLINE bool operator!=(const LabelMapIterator& other) const {
        PLY_ASSERT(this->map == other.map);
        return this->index != other.index;
    }

    PLY_INLINE Item operator*() const {
        u32 mask = this->map->capacity - 1;
        PLY_ASSERT(this->index < this->map->capacity);
        Cell* cell = this->map->cells + ((this->index & mask) >> 2);
        u32 cellIdx = (this->index & 3);
        PLY_ASSERT(cell->keys[cellIdx]);
        return {cell->keys[cellIdx], cell->values[cellIdx]};
    }
};

template <typename T>
PLY_INLINE LabelMapIterator<T> LabelMap<T>::begin() {
    LabelMapIterator<T> iter{this, (u32) -1};
    ++iter;
    return iter;
}

template <typename T>
PLY_INLINE LabelMapIterator<T> LabelMap<T>::end() {
    return {this, this->capacity};
}

template <typename T>
PLY_INLINE LabelMapIterator<const T> LabelMap<T>::begin() const {
    LabelMapIterator<const T> iter{this, (u32) -1};
    ++iter;
    return iter;
}

template <typename T>
PLY_INLINE LabelMapIterator<const T> LabelMap<T>::end() const {
    return {this, this->capacity};
}

} // namespace ply
