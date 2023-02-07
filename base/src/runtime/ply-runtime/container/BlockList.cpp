/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {

//--------------------------------------
// BlockList::Footer
//--------------------------------------
void BlockList::Footer::on_ref_count_zero() {
    BlockList::Footer* block_to_free = this;
    while (block_to_free) {
        BlockList::Footer* next_block = block_to_free->next_block.release();
        // The destructor of this->next_block is now trivial, so we can skip it, and
        // when we free the block data from the heap, it also frees the footer.
        Heap.free(block_to_free->bytes);
        if (next_block) {
            next_block->prev_block = nullptr;
            PLY_ASSERT(next_block->ref_count > 0);
            if (--next_block->ref_count != 0)
                break;
        }
        block_to_free = next_block;
    }
}

BlockList::WeakRef BlockList::Footer::weak_ref_to_next() const {
    BlockList::WeakRef result;
    result.block = this->next_block;
    if (result.block) {
        u32 file_delta =
            check_cast<u32>((s64) result.block->file_offset - this->file_offset);
        PLY_ASSERT(file_delta <= this->num_bytes_used);
        result.byte = result.block->bytes + (this->num_bytes_used - file_delta);
        PLY_ASSERT(result.block->view_used_bytes().contains(result.byte));
    }
    return result;
}

//-------------------------------------------------
// BlockList static member functions
//-------------------------------------------------
Reference<BlockList::Footer> BlockList::create_block(u32 num_bytes) {
    PLY_ASSERT(num_bytes > 100);
    u32 aligned_num_bytes =
        align_power_of2(num_bytes, (u32) alignof(BlockList::Footer));
    u32 alloc_size = aligned_num_bytes + sizeof(BlockList::Footer);
    char* bytes = (char*) Heap.alloc(alloc_size);
    BlockList::Footer* block = (BlockList::Footer*) (bytes + aligned_num_bytes);
    new (block) BlockList::Footer; // Construct in-place
    block->bytes = bytes;
    block->block_size = num_bytes;
    return block;
}

Reference<BlockList::Footer> BlockList::create_overlay_block(const WeakRef& pos,
                                                             u32 num_bytes) {
    Reference<Footer> new_block = create_block(num_bytes);
    new_block->file_offset = pos.block->file_offset + pos.block->offset_of(pos.byte);
    new_block->next_block = pos.block->next_block;
    return new_block;
}

BlockList::Footer* BlockList::append_block(Footer* block, u32 num_bytes) {
    PLY_ASSERT(block->next_block.is_empty());

    block->next_block = create_block(num_bytes);
    block->next_block->file_offset = block->file_offset + block->num_bytes_used;
    block->next_block->prev_block = block;
    return block->next_block;
}

void BlockList::append_block_with_recycle(Reference<Footer>& block, u32 num_bytes) {
    PLY_ASSERT(block->next_block.is_empty());

    // When possible, just reuse the existing block.
    if (block->ref_count == 1 && block->block_size == num_bytes) {
        block->file_offset += block->num_bytes_used;
        block->num_bytes_used = 0;
        return;
    }

    Reference<Footer> new_block = create_block(num_bytes);
    block->next_block = new_block;
    new_block->file_offset = block->file_offset + block->num_bytes_used;
    new_block->prev_block = block;
    block = std::move(new_block);
}

u32 BlockList::jump_to_next_block(WeakRef* weak_ref) {
    PLY_ASSERT(weak_ref->byte = weak_ref->block->unused());
    if (!weak_ref->block->next_block)
        return 0;
    weak_ref->block = weak_ref->block->next_block;
    weak_ref->byte = weak_ref->block->start();
    return weak_ref->block->view_used_bytes().num_bytes;
}

u32 BlockList::jump_to_prev_block(WeakRef* weak_ref) {
    PLY_ASSERT(weak_ref->byte = weak_ref->block->start());
    if (!weak_ref->block->prev_block)
        return 0;
    weak_ref->block = weak_ref->block->prev_block;
    weak_ref->byte = weak_ref->block->unused();
    return weak_ref->block->view_used_bytes().num_bytes;
}

String BlockList::to_string(Ref&& start, const WeakRef& end) {
    PLY_ASSERT(start.byte);

    // Check for special case: When there is only one block and only one reference to
    // the block, we can truncate this block and return it as a String directly.
    if (!start.block->next_block && start.block->ref_count == 1 &&
        start.block->bytes == start.byte) {
        u32 num_bytes = start.block->num_bytes_used;
        char* bytes = (char*) Heap.realloc(start.byte, num_bytes);
        String result = String::adopt(bytes, num_bytes);
        start.block.release();
        start.byte = nullptr;
        return result;
    }

    // Count the total number of bytes.
    u32 num_bytes = 0;
    for (StringView view : iterate_over_views(start, end)) {
        u32 sum = num_bytes + view.num_bytes;
        PLY_ASSERT(sum >= num_bytes); // overflow check
        num_bytes = sum;
    }

    // Allocate a new String and copy data into it.
    String result = String::allocate(num_bytes);
    u32 offset = 0;
    for (StringView view : iterate_over_views(start, end)) {
        memcpy(result.bytes + offset, view.bytes, view.num_bytes);
        offset += view.num_bytes;
    }
    return result;
}

//--------------------------------------
// BlockList object
//--------------------------------------
BlockList::BlockList() {
    this->head = BlockList::create_block();
    this->tail = this->head;
}

BlockList::~BlockList() {
}

char* BlockList::append_bytes(u32 num_bytes) {
    if (this->tail->view_unused_bytes().num_bytes < num_bytes) {
        this->tail = BlockList::append_block(
            this->tail, max(num_bytes, BlockList::DefaultBlockSize));
        PLY_ASSERT(this->tail->view_unused_bytes().num_bytes >= num_bytes);
    }
    char* result = this->tail->unused();
    this->tail->num_bytes_used += num_bytes;
    return result;
}

void BlockList::pop_last_bytes(u32 num_bytes) {
    PLY_ASSERT(this->tail->num_bytes_used >= num_bytes);
    this->tail->num_bytes_used -= num_bytes;
    if ((this->tail->num_bytes_used == 0) && (this->head != this->tail)) {
        this->tail = this->tail->prev_block;
        this->tail->next_block.clear();
    }
}

} // namespace ply
