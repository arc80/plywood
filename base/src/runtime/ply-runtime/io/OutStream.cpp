/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

namespace ply {

OutStream::OutStream(OutStream&& other) : status{other.status} {
    this->start_byte = other.start_byte;
    this->cur_byte = other.cur_byte;
    this->end_byte = other.end_byte;
    this->block = std::move(other.block);
    this->head_block = std::move(other.head_block);
    this->out_pipe = other.out_pipe;
    this->status = other.status;
    other.start_byte = nullptr;
    other.cur_byte = nullptr;
    other.end_byte = nullptr;
    other.out_pipe = nullptr;
    other.status = {};
}

OutStream::OutStream(OutPipe* out_pipe, bool is_pipe_owner) {
    this->status.is_pipe_owner = is_pipe_owner;
    this->out_pipe = out_pipe;

    if (out_pipe) {
        // Init first block.
        this->block = BlockList::create_block(this->status.block_size);
        this->start_byte = this->block->bytes;
        this->cur_byte = this->block->bytes;
        this->end_byte = this->block->end();
    }
}

OutStream::~OutStream() {
    this->flush();
    if (this->status.is_pipe_owner) {
        delete this->out_pipe;
    }
}

u64 OutStream::get_seek_pos() const {
    u64 relative_to = this->block ? this->block->file_offset : 0;
    return relative_to + (this->cur_byte - this->start_byte);
}

void OutStream::flush(bool hard) {
    if (this->block) {
        if (this->out_pipe) {
            // Write buffered data to the pipe.
            if (!this->out_pipe->write(
                    StringView::from_range(this->block->unused(), this->cur_byte))) {
                this->status.eof = 1;
            }
            // Forward flush command down the output chain.
            this->out_pipe->flush(hard);
        }

        // Update the block's write position.
        this->block->num_bytes_used =
            check_cast<u32>(this->cur_byte - this->start_byte);
    }
}

bool OutStream::make_writable() {
    if (this->cur_byte < this->end_byte)
        return true; // We already have writable space.

    if (!this->block)
        return false; // No blocks. We reached the end of a MutStringView.

    // Consistency checks. Existing pointers should match the current block.
    PLY_ASSERT(this->start_byte == this->block->bytes);
    PLY_ASSERT(this->end_byte == this->block->bytes + this->block->block_size);

    bool rc = true;
    if (this->out_pipe) {
        // Write buffered data to the pipe.
        rc = this->out_pipe->write(
            StringView::from_range(this->block->unused(), this->cur_byte));
    }
    // Update the block's write position.
    this->block->num_bytes_used = check_cast<u32>(this->cur_byte - this->start_byte);
    if (!rc) {
        this->status.eof = 1;
        return false;
    }

    // Append a new block.
    BlockList::append_block_with_recycle(this->block, this->status.block_size);
    this->start_byte = this->block->bytes;
    this->cur_byte = this->block->bytes;
    this->end_byte = this->block->bytes + this->block->block_size;
    return true;
}

OutStream& OutStream::operator<<(StringView src) {
    // Loop over the input in case we need to copy data to different blocks.
    while (src.num_bytes > 0) {
        if (!this->ensure_writable())
            return *this; // EOF

        // Copy as much data as possible to the current block.
        u32 to_copy = min<u32>(this->num_writable_bytes(), src.num_bytes);
        memcpy(this->cur_byte, src.bytes, to_copy);
        this->cur_byte += to_copy;
        src.offset_head(to_copy);
    }

    return *this;
}

String OutStream::move_to_string() {
    // Must be a MemOutStream.
    PLY_ASSERT(this->head_block);

    // Update the current block's write position.
    this->block->num_bytes_used = check_cast<u32>(this->cur_byte - this->start_byte);

    // Release block references and create the String. Releasing the references
    // allows BlockList::to_string() to optimize the case where there's only one
    // block.
    this->block = nullptr;
    char* bytes = this->head_block->bytes;
    String result = BlockList::to_string({std::move(this->head_block), bytes});

    // Reset to an empty stream.
    this->close();

    return result;
}

// ┏━━━━━━━━━━━━━━━━┓
// ┃  MemOutStream  ┃
// ┗━━━━━━━━━━━━━━━━┛
MemOutStream::MemOutStream() {
    // Init first block.
    this->head_block = BlockList::create_block(this->status.block_size);
    this->block = this->head_block;
    this->start_byte = this->block->bytes;
    this->cur_byte = this->block->bytes;
    this->end_byte = this->block->end();
}

// ┏━━━━━━━━━━━━━━━━━┓
// ┃  ViewOutStream  ┃
// ┗━━━━━━━━━━━━━━━━━┛
ViewOutStream::ViewOutStream(MutStringView view) {
    this->start_byte = view.bytes;
    this->cur_byte = view.bytes;
    this->end_byte = view.end();
}

} // namespace ply
