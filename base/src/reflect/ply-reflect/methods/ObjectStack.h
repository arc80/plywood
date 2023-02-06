/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/AnyObject.h>

namespace ply {

struct TypeDescriptor;

struct ObjectStack {
    struct Boundary {
        BlockList::WeakRef storage;
        WeakSequenceRef<AnyObject> item;

        PLY_INLINE bool operator!=(const Boundary& other) const {
            return this->item != other.item;
        }
    };

    BlockList storage;
    Sequence<AnyObject> items;

    AnyObject* append_object(TypeDescriptor* type);
    void pop_last_object();
    void delete_range(const Boundary& from, const WeakSequenceRef<AnyObject>& to);
    PLY_INLINE Boundary end() {
        return {this->storage.end(), this->items.end()};
    }
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES
