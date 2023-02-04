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

AnyObject* ObjectStack::appendObject(TypeDescriptor* type) {
    void* data = this->storage.appendBytes(type->fixedSize);
    AnyObject* obj = &this->items.append(data, type);
    obj->construct();
    return obj;
}

void ObjectStack::popLastObject() {
    AnyObject* lastObj = &this->items.tail();
    PLY_ASSERT(PLY_PTR_OFFSET(lastObj->data, lastObj->type->fixedSize) ==
               this->storage.tail->unused());
    lastObj->destruct();
    this->storage.popLastBytes(lastObj->type->fixedSize);
    this->items.popTail();
}

void ObjectStack::deleteRange(const ObjectStack::Boundary& from, const WeakSequenceRef<AnyObject>& to) {
    // Destruct items in the specified range.
    WeakSequenceRef<AnyObject> cur = from.item.normalized();
    for (; cur != to; ++cur) {
        cur->destruct();
    }

    // Move trailing objects into the deleted space.
    BlockList::WeakRef dst = from.storage.normalized();
    WeakSequenceRef<AnyObject> end = this->items.end();
    for (; cur != end; ++cur) {
        u32 numBytes = cur->type->fixedSize;
        if (safeDemote<u32>(dst.block->end() - dst.byte) < numBytes) {
            // FIXME: Handle objects larger than the block size
            PLY_ASSERT(dst.block->nextBlock->blockSize >= numBytes);
            dst = {dst.block->nextBlock, dst.block->bytes};
        }
        // FIXME: Handle alignment
        memmove(dst.byte, cur->data, cur->type->fixedSize);
        cur->data = dst.byte;
        dst.byte += cur->type->fixedSize;
    }

    // Trim storage.
    dst.block->numBytesUsed = dst.block->offsetOf(dst.byte);
    dst.block->nextBlock.clear();
    this->storage.tail = dst.block;

    // Delete items in the specified range.
    WeakSequenceRef<AnyObject> dstItem = from.item.normalized();
    for (WeakSequenceRef<AnyObject> srcItem = to; srcItem != end; ++srcItem) {
        *dstItem = *srcItem;
        ++dstItem;
    }
    this->items.truncate(dstItem);
}

} // namespace ply
