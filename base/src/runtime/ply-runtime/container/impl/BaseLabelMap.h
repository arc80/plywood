/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/Label.h>

namespace ply {
namespace impl {

struct BaseLabelMap {
    struct Cell {
        Label keys[4];
    };

    Cell* cells = nullptr;
    u32 population = 0;
    u32 sizeMask = 0;

    enum Operation { Find, Insert, Erase, Repopulate };

    struct TypeInfo {
        u32 valueSize;
        void (*construct)(void* obj);
        void (*destruct)(void* obj);
        void (*memcpy)(void* dst, void* src);
    };

    template <typename T>
    static TypeInfo typeInfo;
};

template <typename T>
BaseLabelMap::TypeInfo BaseLabelMap::typeInfo = {
    u32{sizeof(T)},
    [](void* obj) { new (obj) T; },
    [](void* obj) { ((T*) obj)->~T(); },
    [](void* dst, void* src) { memcpy(dst, src, sizeof(T)); },
};

void construct(BaseLabelMap* map, const BaseLabelMap::TypeInfo* typeInfo);
void destruct(BaseLabelMap* map, const BaseLabelMap::TypeInfo* typeInfo);
bool operate(BaseLabelMap* map, BaseLabelMap::Operation op, Label key,
             const BaseLabelMap::TypeInfo* typeInfo, void** value);

} // namespace impl
} // namespace ply
