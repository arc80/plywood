/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_Reference;

PLY_DLL_ENTRY NativeBindings& getNativeBindings_Reference();

struct TypeDescriptor_Reference : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;
    void (*incRef)(void* target) = nullptr;
    void (*decRef)(void* target) = nullptr;

    template <typename T>
    PLY_INLINE TypeDescriptor_Reference(T*)
        : TypeDescriptor{&TypeKey_Reference, sizeof(void*), getNativeBindings_Reference()},
          targetType{getTypeDescriptor<T>()} {
        this->incRef = [](void* target) { //
            ((T*) target)->incRef();
        };
        this->decRef = [](void* target) { //
            ((T*) target)->decRef();
        };
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Reference<T>> {
    static PLY_NO_INLINE TypeDescriptor_Reference* get() {
        static TypeDescriptor_Reference typeDesc{(T*) nullptr};
        return &typeDesc;
    }
};

} // namespace ply
