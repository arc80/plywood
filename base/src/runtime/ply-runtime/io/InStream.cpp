/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {

InStream::InStream(InStream&& other) : status{other.status} {
    this->start_byte = other.start_byte;
    this->cur_byte = other.cur_byte;
    this->end_byte = other.end_byte;
    this->block = std::move(other.block);
    this->in_pipe = other.in_pipe;
    this->status = other.status;
    other.start_byte = nullptr;
    other.cur_byte = nullptr;
    other.end_byte = nullptr;
    other.in_pipe = nullptr;
    other.status = {};
}

InStream::InStream(InPipe* in_pipe, bool is_pipe_owner) {
    this->status.is_pipe_owner = is_pipe_owner;
    this->in_pipe = in_pipe;

    if (in_pipe) {
        // Init first block.
        this->block = BlockList::createBlock(this->status.block_size);
        this->start_byte = this->block->bytes;
        this->cur_byte = this->block->bytes;
        this->end_byte = this->block->bytes;
    }
}

InStream::~InStream() {
    if (this->status.is_pipe_owner) {
        delete this->in_pipe;
    }
}

bool InStream::load_more_data() {
    if (this->cur_byte < this->end_byte)
        return true; // More data is available.

    if (!this->block) {
        this->status.eof = true;
        return false; // No blocks. We reached the end of a StringView.
    }

    // Consistency checks. Existing pointers should match the current block.
    PLY_ASSERT(this->start_byte == this->block->bytes);
    PLY_ASSERT(this->end_byte == this->block->unused());

    // Does the current block have unused space?
    if (this->block->numBytesUsed >= this->block->blockSize) {
        // The current block is out of space.
        if (this->block->nextBlock) {
            // The next block already exists. Can happen after rewinding.
            this->block = this->block->nextBlock;
        } else {
            // Append a new empty block.
            BlockList::appendBlockWithRecycle(this->block, this->status.block_size);
        }

        // Expose the new block's data.
        this->start_byte = this->block->bytes;
        this->cur_byte = this->block->bytes;
        this->end_byte = this->block->bytes + this->block->numBytesUsed;

        // If there is already readable data, such as after rewinding, we are done.
        if (this->cur_byte < this->end_byte)
            return true;
    }

    // Load data into the current block's unused space.
    PLY_ASSERT(this->block->numBytesUsed < this->block->blockSize);
    u32 num_bytes_loaded = this->in_pipe->read(this->block->viewUnusedBytes());
    this->block->numBytesUsed += num_bytes_loaded;
    this->end_byte += num_bytes_loaded;
    if (num_bytes_loaded == 0) {
        this->status.eof = true;
    }
    return num_bytes_loaded > 0;
}

u64 InStream::get_seek_pos() const {
    u64 relative_to = this->block ? this->block->fileOffset : 0;
    return relative_to + (this->cur_byte - this->start_byte);
}

void InStream::rewind(const BlockList::WeakRef& pos) {
    this->cur_byte = pos.byte;
    if (pos.block) {
        PLY_ASSERT(this->block);
        this->block = pos.block;
        this->start_byte = pos.block->bytes;
        this->end_byte = pos.block->bytes + pos.block->numBytesUsed;
    } else {
        PLY_ASSERT(!this->block);
    }
    PLY_ASSERT(this->cur_byte >= this->start_byte);
    PLY_ASSERT(this->cur_byte < this->end_byte);
    this->status.eof = 0;
}

char InStream::read_byte_internal() {
    if (this->ensure_readable()) {
        return *this->cur_byte++;
    }
    return 0;
}

bool InStream::read_internal(MutStringView dst) {
    while (dst.numBytes > 0) {
        if (!this->load_more_data()) {
            memset(dst.bytes, 0, dst.numBytes);
            return false;
        }
        u32 toCopy = min<u32>(dst.numBytes, this->num_bytes_readable());
        memcpy(dst.bytes, this->cur_byte, toCopy);
        this->cur_byte += toCopy;
        dst.offsetHead(toCopy);
    }

    return true;
}

String InStream::read_remaining_contents() {
    BlockList::Ref save_point = this->get_save_point();
    while (this->ensure_readable()) {
        this->cur_byte = this->end_byte;
    }
    PLY_ASSERT(this->status.eof);
    this->close();
    return BlockList::toString(std::move(save_point));
}

ViewInStream::ViewInStream(StringView view) {
    this->start_byte = view.bytes;
    this->cur_byte = view.bytes;
    this->end_byte = view.end();
}

} // namespace ply
