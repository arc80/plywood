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

PLY_DLL_ENTRY extern TypeKey TypeKey_Owned;

PLY_DLL_ENTRY NativeBindings& get_native_bindings_owned();

struct TypeDescriptor_Owned : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* type_key;
    TypeDescriptor* target_type;
    void (*assign_raw_ptr)(AnyObject owned_ptr, AnyObject target) = nullptr;

    template <typename T>
    PLY_INLINE TypeDescriptor_Owned(T*)
        : TypeDescriptor{&TypeKey_Owned, (void**) nullptr,
                         get_native_bindings_owned() PLY_METHOD_TABLES_ONLY(, {})},
          target_type{get_type_descriptor<T>()} {
        assign_raw_ptr = [](AnyObject owned_ptr, AnyObject target) {
            // FIXME: Check that the target type is compatible
            // If the target uses multiple inheritance, may need to adjust its pointer
            // value to store as a base class object!
            *reinterpret_cast<Owned<T>*>(owned_ptr.data) = (T*) target.data;
        };
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Owned<T>> {
    static PLY_NO_INLINE TypeDescriptor_Owned* get() {
        static TypeDescriptor_Owned type_desc{(T*) nullptr};
        return &type_desc;
    }
};

} // namespace ply
