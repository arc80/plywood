﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/MethodTable.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/methods/BoundMethod.h>

namespace ply {

PLY_DEFINE_TYPE_DESCRIPTOR(BoundMethod) {
    static TypeDescriptor_Struct typeDesc{(BoundMethod*) nullptr, "BoundMethod"};
    return &typeDesc;
}

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES
