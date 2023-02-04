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
AnyOwnedObject import(TypeDescriptor* typeDesc, const pylon::Node* aRoot,
                   const Func<TypeFromName>& typeFromName = {});
void importInto(AnyObject obj, const pylon::Node* aRoot,
                const Func<TypeFromName>& typeFromName = {});

template <typename T>
PLY_INLINE Owned<T> import(const pylon::Node* aRoot,
                           const Func<TypeFromName>& typeFromName = {}) {
    AnyOwnedObject result = import(getTypeDescriptor<T>(), aRoot, typeFromName);
    return result.release<T>();
}

} // namespace pylon
