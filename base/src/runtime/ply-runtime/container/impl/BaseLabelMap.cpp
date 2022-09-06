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
    map->cells = (BaseLabelMap::Cell*) PLY_HEAP.alloc(sizeof(BaseLabelMap::Cell) +
                                                      (typeInfo->valueSize * 4));
    map->population = 0;
    map->sizeMask = 3;
}

void destruct(BaseLabelMap* map, const BaseLabelMap::TypeInfo* typeInfo) {
    PLY_ASSERT(isPowerOf2(map->sizeMask + 1));
    u32 valueSize = typeInfo->valueSize;
    u32 cellSize = sizeof(BaseLabelMap::Cell) + (valueSize * 4);
    u32 numCells = (map->sizeMask + 1) >> 4;
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

PLY_INLINE u32 bestLabelMapSize(u32 population) {
    if (population > 8) {
        return roundUpPowerOf2(u32{(u64{population} * 5) >> 2});
    }
    return (population <= 4) ? 4 : 8;
}

void repopulate(BaseLabelMap* map, const BaseLabelMap::TypeInfo* typeInfo) {
    PLY_ASSERT(map->population <= 0x6666666);
    u32 newSize = bestLabelMapSize(map->population);
    PLY_ASSERT(isPowerOf2(newSize));
    PLY_ASSERT(newSize >= map->population);
    PLY_ASSERT(map->sizeMask + 1 != newSize);

    // Construct new map
    BaseLabelMap newMap;
    u32 valueSize = typeInfo->valueSize;
    u32 cellSize = sizeof(BaseLabelMap::Cell) + valueSize * 4;
    u32 newNumBytes = cellSize * (newSize >> 2);
    newMap.cells = (BaseLabelMap::Cell*) PLY_HEAP.alloc(newNumBytes);
    memset(newMap.cells, 0, newNumBytes);
    newMap.sizeMask = newNumBytes - 1;

    // Repopulate
    u32 numCells = (map->sizeMask + 1) >> 4;
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
    *map = newMap;
}

PLY_NO_INLINE bool operate(BaseLabelMap* map, BaseLabelMap::Operation op, Label key,
                           const BaseLabelMap::TypeInfo* typeInfo, void** value) {
    PLY_ASSERT(isPowerOf2(map->sizeMask + 1));

    if (op == BaseLabelMap::Insert) {
        if (bestLabelMapSize(map->population + 1) > map->sizeMask + 1) {
            repopulate(map, typeInfo);
        }
    }

    u32 index = avalanche(key.idx);
    u32 valueSize = typeInfo->valueSize;
    u32 cellSize = sizeof(BaseLabelMap::Cell) + valueSize * 4;
    while (true) {
        u32 cellIndex = (index & map->sizeMask) >> 2;
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
                return true;
            } else if (op == BaseLabelMap::Erase) {
                PLY_FORCE_CRASH(); // Not implemented yet
            }
        }
        index++;
    }
}

} // namespace impl
} // namespace ply
