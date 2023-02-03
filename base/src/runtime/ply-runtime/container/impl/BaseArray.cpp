/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime.h>

namespace ply {

void BaseArray::alloc(u32 numItems, u32 itemSize) {
    this->allocated = roundUpPowerOf2(numItems);
    this->items = Heap.alloc(ureg(this->allocated) * itemSize);
    this->num_items = numItems;
}

void BaseArray::realloc(u32 numItems, u32 itemSize) {
    this->allocated = roundUpPowerOf2(numItems);
    this->items = Heap.realloc(this->items, ureg(this->allocated) * itemSize);
    this->num_items = numItems;
}

void BaseArray::free() {
    Heap.free(this->items);
}

void BaseArray::reserve(u32 numItems, u32 itemSize) {
    if (numItems > this->allocated) {
        this->allocated = roundUpPowerOf2(numItems); // FIXME: Generalize to other resize strategies?
        this->items = Heap.realloc(this->items, ureg(this->allocated) * itemSize);
    }
}

void BaseArray::reserveIncrement(u32 itemSize) {
    reserve(this->num_items + 1, itemSize);
}

void BaseArray::truncate(u32 itemSize) {
    this->allocated = this->num_items;
    this->items = Heap.realloc(this->items, ureg(this->allocated) * itemSize);
}

} // namespace ply
