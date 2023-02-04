/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_Array;

PLY_DLL_ENTRY NativeBindings& getNativeBindings_Array();

struct TypeDescriptor_Array : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* itemType;

    PLY_INLINE TypeDescriptor_Array(TypeDescriptor* itemType)
        : TypeDescriptor{&TypeKey_Array, (BaseArray*) nullptr,
                         getNativeBindings_Array() PLY_METHOD_TABLES_ONLY(, {})},
          itemType{itemType} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Array<T>> {
    static PLY_NO_INLINE TypeDescriptor_Array* get() {
        static TypeDescriptor_Array typeDesc{getTypeDescriptor<T>()};
        return &typeDesc;
    }
};

} // namespace ply
