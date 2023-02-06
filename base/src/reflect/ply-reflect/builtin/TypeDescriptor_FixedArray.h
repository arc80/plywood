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

PLY_DLL_ENTRY extern TypeKey TypeKey_FixedArray;

PLY_DLL_ENTRY NativeBindings& get_native_bindings_fixed_array();

struct TypeDescriptor_FixedArray : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* type_key;
    TypeDescriptor* item_type;
    u32 num_items;
    u32 stride;

    PLY_INLINE TypeDescriptor_FixedArray(TypeDescriptor* item_type, u32 num_items)
        : TypeDescriptor{&TypeKey_FixedArray, item_type->fixed_size * num_items,
                         item_type->alignment,
                         get_native_bindings_fixed_array()
                             PLY_METHOD_TABLES_ONLY(, {})},
          item_type{item_type}, num_items{num_items}, stride{item_type->fixed_size} {
    }
    PLY_INLINE TypeDescriptor_FixedArray(TypeDescriptor* item_type, u32 num_items,
                                         u32 stride)
        : TypeDescriptor{&TypeKey_FixedArray, stride * num_items, item_type->alignment,
                         get_native_bindings_fixed_array()
                             PLY_METHOD_TABLES_ONLY(, {})},
          item_type{item_type}, num_items{num_items}, stride{stride} {
        PLY_ASSERT(stride >= item_type->fixed_size);
    }
};

// C-style arrays
template <typename T, int num_items>
struct TypeDescriptorSpecializer<T[num_items]> {
    static PLY_NO_INLINE TypeDescriptor_FixedArray* get() {
        static TypeDescriptor_FixedArray type_desc{get_type_descriptor<T>(),
                                                   u32(num_items)};
        return &type_desc;
    }
};

// FixedArray<>
template <typename T, u32 Size>
struct TypeDescriptorSpecializer<FixedArray<T, Size>> {
    static PLY_NO_INLINE TypeDescriptor_FixedArray* get() {
        static TypeDescriptor_FixedArray type_desc{get_type_descriptor<T>(), Size};
        return &type_desc;
    }
};

} // namespace ply
