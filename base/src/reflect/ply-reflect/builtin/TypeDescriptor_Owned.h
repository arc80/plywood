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

PLY_DLL_ENTRY NativeBindings& getNativeBindings_Owned();

struct TypeDescriptor_Owned : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    TypeDescriptor* targetType;
    void (*assignRawPtr)(AnyObject ownedPtr, AnyObject target) = nullptr;

    template <typename T>
    PLY_INLINE TypeDescriptor_Owned(T*)
        : TypeDescriptor{&TypeKey_Owned, (void**) nullptr,
                         getNativeBindings_Owned() PLY_METHOD_TABLES_ONLY(, {})},
          targetType{getTypeDescriptor<T>()} {
        assignRawPtr = [](AnyObject ownedPtr, AnyObject target) {
            // FIXME: Check that the target type is compatible
            // If the target uses multiple inheritance, may need to adjust its pointer value to
            // store as a base class object!
            *reinterpret_cast<Owned<T>*>(ownedPtr.data) = (T*) target.data;
        };
    }
};

template <typename T>
struct TypeDescriptorSpecializer<Owned<T>> {
    static PLY_NO_INLINE TypeDescriptor_Owned* get() {
        static TypeDescriptor_Owned typeDesc{(T*) nullptr};
        return &typeDesc;
    }
};

} // namespace ply
