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
class LabelMap {
private:
    struct Cell {
        Label keys[4];
        T values[4];
    };

    Cell* cells;
    u32 population;
    u32 sizeMask;

    using namespace impl;

public:
    PLY_INLINE LabelMap() {
        PLY_PUN_SCOPE
        construct((BaseLabelMap*) this, &BaseLabelMap::typeInfo<T>);
    }

    PLY_INLINE ~LabelMap() {
        PLY_PUN_SCOPE
        destruct((BaseLabelMap*) this, &BaseLabelMap::typeInfo<T>);
    }

    PLY_INLINE T* find(Label key) {
        PLY_PUN_SCOPE
        T* value;
        operate((BaseLabelMap*) this, BaseLabelMap::Find, key, &BaseLabelMap::typeInfo<T>, &value);
        return value;
    }

    PLY_INLINE const T* find(Label key) const {
        PLY_PUN_SCOPE
        const T* value;
        operate((BaseLabelMap*) this, BaseLabelMap::Find, key, &BaseLabelMap::typeInfo<T>, &value);
        return value;
    }

    PLY_INLINE bool insert(Label key, T** value) {
        PLY_PUN_SCOPE
        return operate((BaseLabelMap*) this, BaseLabelMap::Insert, key, &BaseLabelMap::typeInfo<T>,
                       value);
    }

    struct Item {
        Label key;
        T* value;
    };
    LabelMapIterator<T> begin();
    LabelMapIterator<T> end();
    LabelMapIterator<const T> begin() const;
    LabelMapIterator<const T> end() const;
};

template <typename T>
struct LabelMapIterator {
    LabelMap<T>* map;
    u32 index;

    PLY_INLINE Item operator++() const {
        while (true) {
            this->index++;
            PLY_ASSERT(this->index <= this->map->sizeMask + 1);
            if (this->index > this->map->sizeMask)
                break;
            Cell* cell = this->map->cells + ((this->index & this->map->sizeMask) >> 2);
            if (cell->keys[this->index & 3])
                break;
        }
    }

    bool operator!=(const Iterator& other) const {
        PLY_ASSERT(this->map == other.map);
        return this->index != other.index;
    }

    PLY_INLINE Item operator*() const {
        PLY_ASSERT(this->index <= this->map->sizeMask);
        Cell* cell = this->map->cells + ((this->index & this->map->sizeMask) >> 2);
        u32 cellIdx = (this->index & 3);
        PLY_ASSERT(cell->keys[cellIdx]);
        return {cell->keys[cellIdx], cell->values[cellIdx]};
    }
};

template <typename T>
PLY_INLINE LabelMapIterator<T> LabelMap<T>::begin() {
    LabelMapIterator iter{this, -1};
    ++iter;
    return iter;
}

template <typename T>
PLY_INLINE LabelMapIterator<T> LabelMap<T>::end() {
    return {this, this->sizeMask + 1};
}

template <typename T>
PLY_INLINE LabelMapIterator<const T> LabelMap<T>::begin() const {
    LabelMapIterator iter{this, -1};
    ++iter;
    return iter;
}

template <typename T>
PLY_INLINE LabelMapIterator<const T> LabelMap<T>::end() const {
    return {this, this->sizeMask + 1};
}

} // namespace ply
