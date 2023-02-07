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

extern TypeKey TypeKey_Array;

NativeBindings& get_native_bindings_array();

struct TypeDescriptor_Array : TypeDescriptor {
    static TypeKey* type_key;
    TypeDescriptor* item_type;

    TypeDescriptor_Array(TypeDescriptor* item_type)
        : TypeDescriptor{&TypeKey_Array, (BaseArray*) nullptr,
                         get_native_bindings_array() PLY_METHOD_TABLES_ONLY(, {})},
          item_type{item_type} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Array<T>> {
    static TypeDescriptor_Array* get() {
        static TypeDescriptor_Array type_desc{get_type_descriptor<T>()};
        return &type_desc;
    }
};

} // namespace ply
