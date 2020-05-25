/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptorOwner.h>

namespace ply {

struct SavedTypedPtr {
    OwnTypedPtr owned;
    Reference<TypeDescriptorOwner> typeOwner;

    SavedTypedPtr() {
        typeOwner = TypeOwnerResolver<EmptyType>::get();
        owned.type = typeOwner->getRootType();
    }

    ~SavedTypedPtr() {
        // Need to destroy the OwnTypedPtr first, since it uses a TypeDescriptor that's owned
        // by the TypeDescriptorOwner.
        owned.destroy();
    }

    SavedTypedPtr& operator=(SavedTypedPtr&&) = default;

    TypeOwnerPtr bundle() const {
        PLY_ASSERT(owned.type == typeOwner->getRootType());
        return {owned.ptr, typeOwner};
    }
};

template <>
struct TypeResolver<SavedTypedPtr> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};

} // namespace ply
