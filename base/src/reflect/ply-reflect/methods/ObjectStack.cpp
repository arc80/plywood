/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/ObjectStack.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

AnyObject* ObjectStack::append_object(TypeDescriptor* type) {
    void* data = this->storage.append_bytes(type->fixed_size);
    AnyObject* obj = &this->items.append(data, type);
    obj->construct();
    return obj;
}

void ObjectStack::pop_last_object() {
    AnyObject* last_obj = &this->items.tail();
    PLY_ASSERT(PLY_PTR_OFFSET(last_obj->data, last_obj->type->fixed_size) ==
               this->storage.tail->unused());
    last_obj->destruct();
    this->storage.pop_last_bytes(last_obj->type->fixed_size);
    this->items.pop_tail();
}

void ObjectStack::delete_range(const ObjectStack::Boundary& from,
                               const WeakSequenceRef<AnyObject>& to) {
    // Destruct items in the specified range.
    WeakSequenceRef<AnyObject> cur = from.item.normalized();
    for (; cur != to; ++cur) {
        cur->destruct();
    }

    // Move trailing objects into the deleted space.
    BlockList::WeakRef dst = from.storage.normalized();
    WeakSequenceRef<AnyObject> end = this->items.end();
    for (; cur != end; ++cur) {
        u32 num_bytes = cur->type->fixed_size;
        if (safe_demote<u32>(dst.block->end() - dst.byte) < num_bytes) {
            // FIXME: Handle objects larger than the block size
            PLY_ASSERT(dst.block->next_block->block_size >= num_bytes);
            dst = {dst.block->next_block, dst.block->bytes};
        }
        // FIXME: Handle alignment
        memmove(dst.byte, cur->data, cur->type->fixed_size);
        cur->data = dst.byte;
        dst.byte += cur->type->fixed_size;
    }

    // Trim storage.
    dst.block->num_bytes_used = dst.block->offset_of(dst.byte);
    dst.block->next_block.clear();
    this->storage.tail = dst.block;

    // Delete items in the specified range.
    WeakSequenceRef<AnyObject> dst_item = from.item.normalized();
    for (WeakSequenceRef<AnyObject> src_item = to; src_item != end; ++src_item) {
        *dst_item = *src_item;
        ++dst_item;
    }
    this->items.truncate(dst_item);
}

} // namespace ply
