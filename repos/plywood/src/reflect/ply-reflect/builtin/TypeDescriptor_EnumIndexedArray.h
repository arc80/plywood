/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Enum.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_EnumIndexedArray;

PLY_DLL_ENTRY NativeBindings& getNativeBindings_EnumIndexedArray();

struct TypeDescriptor_EnumIndexedArray : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* itemType;
    TypeDescriptor_Enum* enumType;

    template <typename T, typename EnumType>
    PLY_INLINE TypeDescriptor_EnumIndexedArray(T*, EnumType*)
        : TypeDescriptor{&TypeKey_EnumIndexedArray, (EnumIndexedArray<T, EnumType>*) nullptr,
                         getNativeBindings_EnumIndexedArray() PLY_METHOD_TABLES_ONLY(, {})} {
        itemType = getTypeDescriptor<T>();
        enumType = TypeDescriptorSpecializer<EnumType>::get()->template cast<TypeDescriptor_Enum>();
        PLY_ASSERT(fixedSize == itemType->fixedSize * enumType->identifiers.numItems());
#if PLY_WITH_ASSERTS
        // The enum type must meet the following requirements:
        // Identifiers are sequentially numbered starting with 0.
        for (u32 i = 0; i < enumType->identifiers.numItems(); i++) {
            PLY_ASSERT(enumType->identifiers[i].value == i);
        }
        // The special identifier "Count" is the last one.
        // "Count" is not reflected. Thus, its integer value matches the number of reflected
        // identifiers.
        PLY_ASSERT(enumType->identifiers.numItems() == (u32) EnumType::Count);
#endif
    }
};

template <typename T, typename EnumType>
struct TypeDescriptorSpecializer<EnumIndexedArray<T, EnumType>> {
    static PLY_NO_INLINE TypeDescriptor_EnumIndexedArray* get() {
        static TypeDescriptor_EnumIndexedArray typeDesc{(T*) nullptr, (EnumType*) nullptr};
        return &typeDesc;
    }
};

} // namespace ply
