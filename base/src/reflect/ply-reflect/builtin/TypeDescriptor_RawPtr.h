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

PLY_DLL_ENTRY extern TypeKey TypeKey_RawPtr;

PLY_DLL_ENTRY NativeBindings& getNativeBindings_RawPtr();

struct TypeDescriptor_RawPtr : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;

    TypeDescriptor_RawPtr(TypeDescriptor* targetType)
        : TypeDescriptor{&TypeKey_RawPtr, (void**) nullptr,
                         getNativeBindings_RawPtr() PLY_METHOD_TABLES_ONLY(, {})},
          targetType{targetType} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<T*> {
    static PLY_NO_INLINE TypeDescriptor_RawPtr* get() {
        static TypeDescriptor_RawPtr typeDesc{getTypeDescriptor<T>()};
        return &typeDesc;
    }
};

} // namespace ply
