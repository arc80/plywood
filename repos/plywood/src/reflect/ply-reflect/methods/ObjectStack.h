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
    struct Boundary {
        BlockList::WeakRef storage;
        WeakSequenceRef<AnyObject> item;
    };

    BlockList storage;
    Sequence<AnyObject> items;

    AnyObject* appendObject(TypeDescriptor* type);
    void popLastObject();
    void deleteRange(const Boundary& from, const WeakSequenceRef<AnyObject>& to);
    PLY_INLINE Boundary end() {
        return {this->storage.end(), this->items.end()};
    }
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES
