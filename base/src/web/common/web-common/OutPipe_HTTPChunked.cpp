/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-common/Core.h>
#include <web-common/OutPipe_HTTPChunked.h>

namespace ply {
namespace web {

void OutPipe_HTTPChunked_destroy(OutPipe* out_pipe_) {
    OutPipe_HTTPChunked* out_pipe = static_cast<OutPipe_HTTPChunked*>(out_pipe_);
    // End of chunk stream
    *out_pipe->outs << "0\r\n\r\n";
    out_pipe->outs->flush_mem();
}

bool OutPipe_HTTPChunked_write(OutPipe* out_pipe_, StringView src_buf) {
    OutPipe_HTTPChunked* out_pipe = static_cast<OutPipe_HTTPChunked*>(out_pipe_);
    if (out_pipe->chunk_mode) {
        PLY_ASSERT(src_buf.num_bytes > 0);
        out_pipe->outs->format("{}\r\n", fmt::Hex{src_buf.num_bytes, true});
    }
    out_pipe->outs->write(src_buf);
    if (out_pipe->chunk_mode) {
        *out_pipe->outs << "\r\n";
    }
    return !out_pipe->outs->at_eof();
}

bool OutPipe_HTTPChunked_flush(OutPipe* out_pipe_, bool to_device) {
    OutPipe_HTTPChunked* out_pipe = static_cast<OutPipe_HTTPChunked*>(out_pipe_);
    return out_pipe->outs->flush(to_device);
}

OutPipe::Funcs OutPipe_HTTPChunked::Funcs_ = {
    OutPipe_HTTPChunked_destroy,
    OutPipe_HTTPChunked_write,
    OutPipe_HTTPChunked_flush,
    OutPipe::seek_Empty,
};

} // namespace web
} // namespace ply
