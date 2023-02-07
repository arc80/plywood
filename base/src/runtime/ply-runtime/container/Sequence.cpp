/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

namespace ply {
namespace impl {

void destruct_sequence(Reference<BlockList::Footer>* head_ref,
                       void (*destruct_view_as)(StringView)) {
    BlockList::Footer* block_to_free = head_ref->release();
    while (block_to_free) {
        // There should be only own reference: the Sequence.
        PLY_ASSERT(block_to_free->ref_count == 1);
        destruct_view_as(block_to_free->view_used_bytes());
        BlockList::Footer* next_block = block_to_free->next_block.release();
        // The destructor of this->next_block is now trivial, so we can skip it, and
        // when we free the block data from the heap, it also frees the footer.
        Heap.free(block_to_free->bytes);
        block_to_free = next_block;
    }
}

void begin_write_internal(BlockList::Footer** tail, u32 num_bytes) {
    PLY_ASSERT((*tail)->view_unused_bytes().num_bytes < num_bytes);
    *tail = BlockList::append_block(*tail, max(BlockList::DefaultBlockSize, num_bytes));
}

void pop_tail(BlockList::Footer** tail, u32 num_bytes,
              void (*destruct_view_as)(StringView)) {
    BlockList::Footer* block = *tail;
    while (num_bytes > 0) {
        u32 bytes_to_pop = min(num_bytes, block->view_used_bytes().num_bytes);
        // It is illegal to attempt to pop more items than the sequence contains.
        PLY_ASSERT(bytes_to_pop > 0);
        destruct_view_as({block->unused() - bytes_to_pop, bytes_to_pop});
        block->num_bytes_used -= bytes_to_pop;
        num_bytes -= bytes_to_pop;
        if (block->view_used_bytes().num_bytes > 0) {
            PLY_ASSERT(num_bytes == 0);
            break;
        }
        block = block->prev_block;
        if (!block)
            break;
        *tail = block;
        block->next_block.clear();
    }
}

void truncate(BlockList::Footer** tail, const BlockList::WeakRef& to) {
    if (to.byte == to.block->start() && to.block->prev_block) {
        *tail = to.block->prev_block;
    } else {
        *tail = to.block;
        to.block->num_bytes_used = to.block->offset_of(to.byte);
    }
    (*tail)->next_block.clear();
}

u32 get_total_num_bytes(BlockList::Footer* head) {
    u32 num_bytes = 0;
    while (head) {
        num_bytes += head->view_used_bytes().num_bytes;
        head = head->next_block;
    }
    return num_bytes;
}

char* read(BlockList::WeakRef* weak_ref, u32 item_size) {
    sptr num_bytes_available = weak_ref->block->unused() - weak_ref->byte;
    // It's illegal to call this function at the end of a sequence.
    PLY_ASSERT(num_bytes_available >= item_size);
    char* result = weak_ref->byte;
    weak_ref->byte += item_size;
    num_bytes_available -= item_size;
    if (num_bytes_available == 0) {
        num_bytes_available = BlockList::jump_to_next_block(weak_ref);
        // We might now be at the end of the sequence.
    } else {
        // num_bytes_available should always be a multiple of the item size.
        PLY_ASSERT(num_bytes_available >= item_size);
    }
    return result;
}

} // namespace impl
} // namespace ply
