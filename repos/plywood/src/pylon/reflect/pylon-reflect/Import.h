/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <pylon-reflect/Core.h>
#include <pylon/Node.h>
#include <ply-reflect/TypeDescriptor.h>
#include <ply-reflect/TypeKey.h>
#include <ply-runtime/container/Owned.h>

namespace pylon {

typedef TypeDescriptor* TypeFromName(StringView);
OwnTypedPtr import(TypeDescriptor* typeDesc, const pylon::Node* aRoot,
                   const Functor<TypeFromName>& typeFromName = {});
void importInto(TypedPtr obj, const pylon::Node* aRoot,
                const Functor<TypeFromName>& typeFromName = {});

template <typename T>
PLY_INLINE Owned<T> import(const pylon::Node* aRoot,
                           const Functor<TypeFromName>& typeFromName = {}) {
    OwnTypedPtr result = import(TypeResolver<T>::get(), aRoot, typeFromName);
    return result.release<T>();
}

} // namespace pylon
