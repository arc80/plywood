﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_RawPtr;

PLY_DLL_ENTRY NativeBindings& get_native_bindings_raw_ptr();

struct TypeDescriptor_RawPtr : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* type_key;
    TypeDescriptor* target_type;

    TypeDescriptor_RawPtr(TypeDescriptor* target_type)
        : TypeDescriptor{&TypeKey_RawPtr, (void**) nullptr,
                         get_native_bindings_raw_ptr() PLY_METHOD_TABLES_ONLY(, {})},
          target_type{target_type} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<T*> {
    static PLY_NO_INLINE TypeDescriptor_RawPtr* get() {
        static TypeDescriptor_RawPtr type_desc{get_type_descriptor<T>()};
        return &type_desc;
    }
};

} // namespace ply
