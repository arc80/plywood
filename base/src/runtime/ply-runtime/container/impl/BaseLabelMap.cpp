/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/impl/BaseLabelMap.h>

namespace ply {
namespace impl {

void construct(BaseLabelMap* map, const BaseLabelMap::TypeInfo* typeInfo) {
    new (map) BaseLabelMap;
    u32 numBytes = sizeof(BaseLabelMap::Cell) + (typeInfo->valueSize * 4);
    map->cells = (BaseLabelMap::Cell*) PLY_HEAP.alloc(numBytes);
    memset(map->cells, 0, numBytes);
    map->population = 0;
    map->capacity = 4;
}

void destruct(BaseLabelMap* map, const BaseLabelMap::TypeInfo* typeInfo) {
    PLY_ASSERT(isPowerOf2(map->capacity));
    u32 valueSize = typeInfo->valueSize;
    u32 cellSize = sizeof(BaseLabelMap::Cell) + (valueSize * 4);
    u32 numCells = map->capacity >> 2;
    BaseLabelMap::Cell* endCell =
        (BaseLabelMap::Cell*) PLY_PTR_OFFSET(map->cells, numCells * cellSize);
    BaseLabelMap::Cell* cell = map->cells;
    while (cell < endCell) {
        for (u32 i = 0; i < 4; i++) {
            if (cell->keys[i]) {
                typeInfo->destruct(PLY_PTR_OFFSET(cell + 1, valueSize * i));
            }
        }
        cell = (BaseLabelMap::Cell*) PLY_PTR_OFFSET(cell, cellSize);
    }
    PLY_HEAP.free(map->cells);
}

PLY_INLINE u32 bestLabelMapCapacity(u32 population) {
    if (population > 8) {
        return roundUpPowerOf2(u32((u64{population} * 5) >> 2));
    }
    return (population < 4) ? 4 : 8;
}

void repopulate(BaseLabelMap* map, u32 newCapacity, const BaseLabelMap::TypeInfo* typeInfo) {
    PLY_ASSERT(map->population <= 0x6666666);
    PLY_ASSERT(map->capacity != newCapacity);
    PLY_ASSERT(isPowerOf2(newCapacity));
    PLY_ASSERT(newCapacity >= map->population);

    // Construct new map
    BaseLabelMap newMap;
    u32 valueSize = typeInfo->valueSize;
    u32 cellSize = sizeof(BaseLabelMap::Cell) + valueSize * 4;
    u32 newNumBytes = cellSize * (newCapacity >> 2);
    newMap.cells = (BaseLabelMap::Cell*) PLY_HEAP.alloc(newNumBytes);
    memset(newMap.cells, 0, newNumBytes);
    newMap.population = map->population;
    newMap.capacity = newCapacity;

    // Repopulate
    u32 numCells = map->capacity >> 2;
    BaseLabelMap::Cell* endCell =
        (BaseLabelMap::Cell*) PLY_PTR_OFFSET(map->cells, numCells * cellSize);
    BaseLabelMap::Cell* cell = map->cells;
    while (cell < endCell) {
        for (u32 i = 0; i < 4; i++) {
            if (cell->keys[i]) {
                void* value;
                operate(&newMap, BaseLabelMap::Repopulate, cell->keys[i], typeInfo, &value);
                typeInfo->memcpy(value, PLY_PTR_OFFSET(cell + 1, valueSize * i));
            }
        }
        cell = (BaseLabelMap::Cell*) PLY_PTR_OFFSET(cell, cellSize);
    }

    PLY_HEAP.free(map->cells);
    *map = newMap; // Copy each member to the previous map
}

PLY_NO_INLINE bool operate(BaseLabelMap* map, BaseLabelMap::Operation op, Label key,
                           const BaseLabelMap::TypeInfo* typeInfo, void** value) {
    PLY_ASSERT(isPowerOf2(map->capacity));
    PLY_ASSERT(map->population <= map->capacity);

    if (op == BaseLabelMap::Insert) {
        u32 minCapacity = bestLabelMapCapacity(map->population + 1);
        if (map->capacity < minCapacity) {
            repopulate(map, minCapacity, typeInfo);
        }
    }

    u32 index = avalanche(key.idx);
    u32 valueSize = typeInfo->valueSize;
    u32 cellSize = sizeof(BaseLabelMap::Cell) + valueSize * 4;
    u32 mask = map->capacity - 1;
    while (true) {
        u32 cellIndex = (index & mask) >> 2;
        BaseLabelMap::Cell* cell =
            (BaseLabelMap::Cell*) PLY_PTR_OFFSET(map->cells, cellSize * cellIndex);
        u32 indexInCell = (index & 3);
        if (!cell->keys[indexInCell]) {
            if (op == BaseLabelMap::Find) {
                *value = nullptr;
                return false;
            } else if (op == BaseLabelMap::Insert) {
                cell->keys[indexInCell] = key;
                *value = PLY_PTR_OFFSET(cell + 1, valueSize * indexInCell);
                typeInfo->construct(*value);
                map->population++;
                return true;
            } else if (op == BaseLabelMap::Repopulate) {
                cell->keys[indexInCell] = key;
                *value = PLY_PTR_OFFSET(cell + 1, valueSize * indexInCell);
                return true;
            } else if (op == BaseLabelMap::Erase) {
                PLY_FORCE_CRASH(); // Not implemented yet
            }
        }
        if (cell->keys[indexInCell] == key) {
            if (op == BaseLabelMap::Find) {
                *value = PLY_PTR_OFFSET(cell + 1, valueSize * indexInCell);
                return true;
            } else if (op == BaseLabelMap::Insert) {
                *value = PLY_PTR_OFFSET(cell + 1, valueSize * indexInCell);
                return false;
            } else if (op == BaseLabelMap::Repopulate) {
                PLY_ASSERT(0); // Shouldn't happen
            } else if (op == BaseLabelMap::Erase) {
                PLY_FORCE_CRASH(); // Not implemented yet
            }
        }
        index++;
    }
}

} // namespace impl
} // namespace ply
