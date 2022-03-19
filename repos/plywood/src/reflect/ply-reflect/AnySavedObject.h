/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptorOwner.h>
#include <ply-reflect/AnyOwnedObject.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_AnySavedObject;

struct AnySavedObject {
    AnyOwnedObject owned;
    Reference<TypeDescriptorOwner> typeOwner;

    AnySavedObject() {
        typeOwner = TypeOwnerResolver<EmptyType>::get();
        owned.type = typeOwner->getRootType();
    }

    ~AnySavedObject() {
        // Need to destroy the AnyOwnedObject first, since it uses a TypeDescriptor that's owned
        // by the TypeDescriptorOwner.
        owned.destroy();
    }

    AnySavedObject& operator=(AnySavedObject&&) = default;

    TypeOwnerPtr bundle() const {
        PLY_ASSERT(owned.type == typeOwner->getRootType());
        return {owned.data, typeOwner};
    }
};

PLY_DECLARE_TYPE_DESCRIPTOR(AnySavedObject, PLY_DLL_ENTRY)

} // namespace ply
