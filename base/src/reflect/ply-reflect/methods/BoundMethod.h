/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
