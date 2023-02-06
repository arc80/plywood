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

PLY_DLL_ENTRY extern TypeKey TypeKey_Reference;

PLY_DLL_ENTRY NativeBindings& get_native_bindings_reference();

struct TypeDescriptor_Reference : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* type_key;
    TypeDescriptor* target_type;
    void (*inc_ref)(void* target) = nullptr;
    void (*dec_ref)(void* target) = nullptr;

    template <typename T>
    PLY_INLINE TypeDescriptor_Reference(T*)
        : TypeDescriptor{&TypeKey_Reference, (void**) nullptr,
                         get_native_bindings_reference() PLY_METHOD_TABLES_ONLY(, {})},
          target_type{get_type_descriptor<T>()} {
        this->inc_ref = [](void* target) { //
            ((T*) target)->inc_ref();
        };
        this->dec_ref = [](void* target) { //
            ((T*) target)->dec_ref();
        };
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Reference<T>> {
    static PLY_NO_INLINE TypeDescriptor_Reference* get() {
        static TypeDescriptor_Reference type_desc{(T*) nullptr};
        return &type_desc;
    }
};

} // namespace ply
