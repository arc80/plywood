/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/AnyObject.h>
#include <ply-reflect/builtin/TypeDescriptor_Struct.h>

namespace ply {

struct BoundMethod {
    AnyObject target;
    AnyObject func;
};

PLY_DECLARE_TYPE_DESCRIPTOR(BoundMethod)

} // namespace ply
