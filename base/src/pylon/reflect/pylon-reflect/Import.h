/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <pylon-reflect/Core.h>
#include <pylon/Node.h>
#include <ply-reflect/TypeDescriptor.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/AnyOwnedObject.h>

namespace pylon {

typedef TypeDescriptor* TypeFromName(StringView);
AnyOwnedObject import(TypeDescriptor* type_desc, const pylon::Node* a_root,
                      const Func<TypeFromName>& type_from_name = {});
void import_into(AnyObject obj, const pylon::Node* a_root,
                 const Func<TypeFromName>& type_from_name = {});

template <typename T>
PLY_INLINE Owned<T> import(const pylon::Node* a_root,
                           const Func<TypeFromName>& type_from_name = {}) {
    AnyOwnedObject result = import(get_type_descriptor<T>(), a_root, type_from_name);
    return result.release<T>();
}

} // namespace pylon
