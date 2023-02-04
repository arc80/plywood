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

PLY_DLL_ENTRY NativeBindings& getNativeBindings_FixedArray();

struct TypeDescriptor_FixedArray : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* itemType;
    u32 numItems;
    u32 stride;

    PLY_INLINE TypeDescriptor_FixedArray(TypeDescriptor* itemType, u32 numItems)
        : TypeDescriptor{&TypeKey_FixedArray, itemType->fixedSize * numItems, itemType->alignment,
                         getNativeBindings_FixedArray() PLY_METHOD_TABLES_ONLY(, {})},
          itemType{itemType}, numItems{numItems}, stride{itemType->fixedSize} {
    }
    PLY_INLINE TypeDescriptor_FixedArray(TypeDescriptor* itemType, u32 numItems, u32 stride)
        : TypeDescriptor{&TypeKey_FixedArray, stride * numItems, itemType->alignment,
                         getNativeBindings_FixedArray() PLY_METHOD_TABLES_ONLY(, {})},
          itemType{itemType}, numItems{numItems}, stride{stride} {
        PLY_ASSERT(stride >= itemType->fixedSize);
    }
};

// C-style arrays
template <typename T, int numItems>
struct TypeDescriptorSpecializer<T[numItems]> {
    static PLY_NO_INLINE TypeDescriptor_FixedArray* get() {
        static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<T>(), u32(numItems)};
        return &typeDesc;
    }
};

// FixedArray<>
template <typename T, u32 Size>
struct TypeDescriptorSpecializer<FixedArray<T, Size>> {
    static PLY_NO_INLINE TypeDescriptor_FixedArray* get() {
        static TypeDescriptor_FixedArray typeDesc{getTypeDescriptor<T>(), Size};
        return &typeDesc;
    }
};

} // namespace ply
