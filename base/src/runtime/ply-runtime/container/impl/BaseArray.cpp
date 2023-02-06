/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {

void BaseArray::alloc(u32 num_items, u32 item_size) {
    this->allocated = round_up_power_of2(num_items);
    this->items = Heap.alloc(ureg(this->allocated) * item_size);
    this->num_items = num_items;
}

void BaseArray::realloc(u32 num_items, u32 item_size) {
    this->allocated = round_up_power_of2(num_items);
    this->items = Heap.realloc(this->items, ureg(this->allocated) * item_size);
    this->num_items = num_items;
}

void BaseArray::free() {
    Heap.free(this->items);
}

void BaseArray::reserve(u32 num_items, u32 item_size) {
    if (num_items > this->allocated) {
        this->allocated = round_up_power_of2(
            num_items); // FIXME: Generalize to other resize strategies?
        this->items = Heap.realloc(this->items, ureg(this->allocated) * item_size);
    }
}

void BaseArray::reserve_increment(u32 item_size) {
    reserve(this->num_items + 1, item_size);
}

void BaseArray::truncate(u32 item_size) {
    this->allocated = this->num_items;
    this->items = Heap.realloc(this->items, ureg(this->allocated) * item_size);
}

} // namespace ply
