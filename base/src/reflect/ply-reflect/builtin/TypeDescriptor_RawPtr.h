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

extern TypeKey TypeKey_RawPtr;

NativeBindings& get_native_bindings_raw_ptr();

struct TypeDescriptor_RawPtr : TypeDescriptor {
    static TypeKey* type_key;
    TypeDescriptor* target_type;

    TypeDescriptor_RawPtr(TypeDescriptor* target_type)
        : TypeDescriptor{&TypeKey_RawPtr, (void**) nullptr,
                         get_native_bindings_raw_ptr() PLY_METHOD_TABLES_ONLY(, {})},
          target_type{target_type} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<T*> {
    static TypeDescriptor_RawPtr* get() {
        static TypeDescriptor_RawPtr type_desc{get_type_descriptor<T>()};
        return &type_desc;
    }
};

} // namespace ply
