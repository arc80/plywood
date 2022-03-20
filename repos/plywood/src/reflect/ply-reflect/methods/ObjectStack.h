/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/AnyObject.h>
#include <ply-runtime/container/Sequence.h>

namespace ply {

struct TypeDescriptor;

struct ObjectStack {
    BlockList storage;
    Sequence<AnyObject> items;

    AnyObject* appendObject(TypeDescriptor* type);
    void popLastObject();
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES
